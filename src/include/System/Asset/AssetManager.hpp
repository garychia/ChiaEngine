#ifndef ASSETMANAGER_HPP
#define ASSETMANAGER_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <mutex>

#include "Data/DynamicArray.hpp"
#include "Data/Hash.hpp"
#include "Data/HashTable.hpp"
#include "Data/Pointers.hpp"
#include "Data/String.hpp"
#include "System/Job/JobSystem.hpp"
#include "System/Operation/Event.hpp"
#include "Types/Types.hpp"

// 共享內容區塊:同一個 contentHash(FNV-1a over bytes)的所有 Asset 共用同一份記憶體。
// 由 SharedPtr refcount 管理,最後一個持有者釋放。TextTexture/Shader 直接吃 block.bytes。
struct AssetBlock
{
    DynamicArray<unsigned char> bytes;
    uint64_t contentHash;

    AssetBlock(const DynamicArray<unsigned char> &inBytes, uint64_t hash)
        : bytes(inBytes), contentHash(hash)
    {
    }
};

// 資產 v2:整包位元組改為「內容定址、跨 key 共享」的 AssetBlock。
// 兩個不同 key 載入 byte-identical 內容 → 指向同一個 AssetBlock(不重複分配)。
// `bytes` 在 v1 是所有權(value copy);v2 交給 block 共享,Asset 只剩存取子。
struct Asset
{
    String key;
    uint64_t keyHash;     // key 的 FNV-1a(HashTable 只支援整數 key → 內容定址)
    SharedPtr<AssetBlock> pBlock; // 內容區塊(可為 null = 尚未載入/載入失敗)
    bool loaded;

    Asset(const String &key, uint64_t keyHash)
        : key(key), keyHash(keyHash), pBlock(), loaded(false)
    {
    }

    // 方便存取(載入成功後才有意義;pBlock 由 LoadWorker 在完成前填好)。
    const DynamicArray<unsigned char> &Bytes() const
    {
        return pBlock->bytes;
    }

    uint64_t ContentHash() const
    {
        return pBlock ? pBlock->contentHash : 0;
    }
};

// AssetManager:HashTable<uint64_t, AssetHandle>(key = key 的 FNV-1a,內容定址)
// 依 key 去重;載入結果依 **contentHash** 共享(見 AssetBlock)。
// 非同步載入完成事件由主執行緒統一廣播(Event 非 thread-safe)。
//
// 使用慣例:
//  - 所有公開方法在「主執行緒」呼叫(LoadAsync / DispatchCompletedEvents / GetLoadedBytes)。
//  - 載入在 JobSystem 的 worker 執行:
//      * 只碰「本次待寫入的 sharedBlock」(讀檔 → 算 contentHash → 衰 mutex 找/建 block)。
//      * `blocks`(contentHash → shared block)是跨 worker 共享表,寫入期間要加鎖。
//  - 完成事件不從 worker 直接發 — 塞進 completed 佇列,
//    DispatchCompletedEvents() 在主執行緒成批廣播 LoadedEvent。
class AssetManager
{
  public:
    typedef SharedPtr<Asset> AssetId;
    typedef SharedPtr<AssetBlock> BlockId;

    explicit AssetManager(JobSystem &jobs) : jobs(jobs), numAssets(0)
    {
    }

    // 完成事件(主執行緒,DispatchCompletedEvents 內觸發)。
    Event<void(AssetId)> LoadedEvent;

    // 非同步請求載入:已載入/載入中 → 回傳既有 id(依 key 去重,不重複讀檔、不重複發事件)。
    AssetId LoadAsync(const String &key)
    {
        const uint64_t keyHash = HashKey(key);
        typename HashTable<uint64_t, AssetId>::Iterator itr = assets.Find(keyHash);
        if (itr != assets.Last())
            return itr->Value();

        AssetId id = SharedPtr<Asset>::Construct(key, keyHash);
        assets.Insert(keyHash, id);
        numAssets++;

        // worker 只碰「建立/共享 block」,Asset 本身由 SharedPtr 持有跨執行緒安全。
        jobs.Enqueue([this, id] { LoadWorker(id); });
        return id;
    }

    // 主執行緒:把完成佇列裡的資產依序廣播 LoadedEvent。回傳本批事件數。
    size_t DispatchCompletedEvents()
    {
        size_t n = 0;
        for (;;)
        {
            AssetId id;
            {
                std::lock_guard<std::mutex> lock(completedJobsMutex);
                if (completedCursor >= completedJobs.Length())
                    break;
                id = completedJobs[completedCursor];
                completedCursor++;
            }
            LoadedEvent.Invoke(id);
            n++;
        }
        // 捨棄已讀的 slot(避免佇列無限長)
        {
            std::lock_guard<std::mutex> lock(completedJobsMutex);
            if (completedCursor == completedJobs.Length())
            {
                completedJobs.RemoveAll();
                completedCursor = 0;
            }
        }
        return n;
    }

    size_t GetNumAssets() const
    {
        return numAssets;
    }

    // 跨 key 共享的內容區塊數(不同 contentHash 的個數)。供測試驗證「相同內容只存一份」。
    size_t GetNumSharedBlocks()
    {
        std::lock_guard<std::mutex> lock(blocksMutex);
        return numSharedBlocks;
    }

    // 已完成且已派發的資產的資料(依 key 查)。找不到回傳 false。
    bool GetLoadedBytes(const String &key, DynamicArray<unsigned char> *pOut)
    {
        typename HashTable<uint64_t, AssetId>::Iterator it = assets.Find(HashKey(key));
        if (it == assets.Last())
            return false;
        AssetId id = it->Value();
        if (!id->loaded || !id->pBlock)
            return false;
        *pOut = id->pBlock->bytes;
        return true;
    }

  private:
    static uint64_t HashKey(const String &key)
    {
        const Str<char> utf8 = key.ToUTF8();
        return FNV1A64(FNV1A64_OFFSET, utf8.CStr(), utf8.Length());
    }

    void LoadWorker(AssetId id)
    {
        id->loaded = false;
        id->pBlock = SharedPtr<AssetBlock>(); // 重置,重新決定共享目標

        const Str<char> utf8Path = id->key.ToUTF8();
        DynamicArray<unsigned char> rawBytes; // 臨時複本,決定要共享哪個 block 後即不再持有
        bool ok = false;
        std::ifstream file(utf8Path.CStr(), std::ios::binary);
        if (file)
        {
            file.seekg(0, std::ios::end);
            const std::streamoff size = file.tellg();
            file.seekg(0, std::ios::beg);
            const size_t n = size > 0 ? static_cast<size_t>(size) : 0;
            rawBytes.Resize(n);
            if (n > 0)
                file.read(reinterpret_cast<char *>(&rawBytes[0]), static_cast<std::streamsize>(n));
            ok = true;
        }

        if (ok)
        {
            const uint64_t contentHash =
                (rawBytes.Length() > 0) ? FNV1A64(FNV1A64_OFFSET, &rawBytes[0], rawBytes.Length())
                                        : FNV1A64_OFFSET;

            // 找既有共享 block(contentHash 相同)。為防 hash 碰撞,再比對 bytes。
            {
                std::lock_guard<std::mutex> lock(blocksMutex);
                typename HashTable<uint64_t, BlockId>::Iterator itr = blocks.Find(contentHash);
                if (itr != blocks.Last())
                {
                    BlockId existing = itr->Value();
                    if (BytesEqual(existing->bytes, rawBytes))
                    {
                        id->pBlock = existing;
                        id->loaded = true;
                    }
                }
                if (!id->pBlock)
                {
                    id->pBlock = SharedPtr<AssetBlock>::Construct(rawBytes, contentHash);
                    blocks.Insert(contentHash, id->pBlock);
                    numSharedBlocks++;
                    id->loaded = true;
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(completedJobsMutex);
            completedJobs.Append(id);
        }
    }

    static bool BytesEqual(const DynamicArray<unsigned char> &lhs, const DynamicArray<unsigned char> &rhs)
    {
        if (lhs.Length() != rhs.Length())
            return false;
        if (lhs.Length() == 0)
            return true;
        return std::memcmp(&lhs[0], &rhs[0], lhs.Length()) == 0;
    }

    JobSystem &jobs;
    HashTable<uint64_t, AssetId> assets;              // key hash -> 資產(依 key 去重)
    size_t numAssets;
    HashTable<uint64_t, BlockId> blocks;              // contentHash -> 共享內容區塊
    size_t numSharedBlocks = 0;
    std::mutex blocksMutex;
    DynamicArray<AssetId> completedJobs;
    size_t completedCursor = 0;
    std::mutex completedJobsMutex;
};

#endif // ASSETMANAGER_HPP