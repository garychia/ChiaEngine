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

// 共享內容區塊:同一顆 contentHash(FNV-1a over bytes)的所有 Asset 共用同一份記憶體。
// #86:不再維護 block 自己的 refCount —— 記憶體存亡完全由 SharedPtr<AssetBlock>(atomic
// 引用計數)決定,最後一份釋放才真正 unload;「目前被幾個已載入 asset 指向」語意保留,
// 但改由 GetBlockRefCount() 掃描 assets 表推得,避免兩套引用計數並存失諧。
struct AssetBlock
{
    DynamicArray<unsigned char> bytes;
    uint64_t contentHash;

    AssetBlock(const DynamicArray<unsigned char> &inBytes, uint64_t hash) : bytes(inBytes), contentHash(hash)
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
    uint32_t refCount;    // 消費方租約:LoadAsync 建立時 = 1,每次 Release 遞減。歸零才卸載。
    bool released;        // 已在載入完成前被 Release(等 Dispatch 時真正 unload)

    Asset(const String &key, uint64_t keyHash)
        : key(key), keyHash(keyHash), pBlock(), loaded(false), refCount(1), released(false)
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
//  - 記憶體存亡 = SharedPtr(atomic 引用計數,#86)→ 跨執行緒複製一律安全、不會雙 delete;
//    Asset 與 AssetBlock 的記憶體生命週期完全由 SharedPtr 決定。
//  - `Asset::refCount` 是「消費方租約」(主執行緒專用,不是記憶體引用數):保證 LoadAsync
//    每一次回傳都有對應的 Release、卸載不早於最後一次 Release(含載入完成前就釋放)。
//  - worker 會寫 id->loaded / id->pBlock —— 主執行緒對這兩個欄位的讀取一律拿 blocksMutex,
//    避免與 worker 交錯時看到半套狀態。
//  - 載入在 JobSystem 的 worker 執行:讀檔 → 算 contentHash → 拿 blocksMutex 找/建 block。
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
    // 每一次呼叫都是一個「消費方引用」:呼叫者要在不再使用時呼叫 Release。
    AssetId LoadAsync(const String &key)
    {
        const uint64_t keyHash = HashKey(key);
        typename HashTable<uint64_t, AssetId>::Iterator itr = assets.Find(keyHash);
        if (itr != assets.Last())
        {
            // key 已存在(載入中或已載入):回傳同一 handle,並為這個新消費方 +1。
            itr->Value()->refCount++;
            return itr->Value();
        }

        AssetId id = SharedPtr<Asset>::Construct(key, keyHash);
        assets.Insert(keyHash, id);
        numAssets++;

        // worker 只碰「建立/共享 block」、寫 id->loaded / id->pBlock(均在 blocksMutex 保護下)。
        jobs.Enqueue([this, id] { LoadWorker(id); });
        return id;
    }

    // 消費方釋放引用:refCount 歸零時才卸載。
    //  - 已載入 → 立即從 assets 表移除、並把 block 的引用還回去。
    //  - 載入中 → 標記 released,由 DispatchCompletedEvents 在完成時真正 evict。
    void Release(AssetId id)
    {
        if (!id || id->refCount == 0)
            return;
        id->refCount--;
        if (id->refCount > 0)
            return;

        id->released = true;
        bool unloadNow = false;
        {
            // #86:loaded/pBlock 由 worker 寫入 → 加鎖讀,避免撕裂/過期判斷。
            std::lock_guard<std::mutex> lock(blocksMutex);
            unloadNow = id->loaded && id->pBlock;
        }
        if (unloadNow)
            UnloadAsset(id);
    }

    // 主執行緒:把完成佇列裡的資產依序廣播 LoadedEvent。
    // 載入完成前已被釋放的資產不發事件,直接 evict。回傳本批事件數。
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
            if (id->released && id->refCount == 0)
                UnloadAsset(id); // 已在載入完成前被釋放 → 不廣播,直接卸載
            else
            {
                LoadedEvent.Invoke(id);
                n++;
            }
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

    // 目前存活的(未釋放的)key 數。
    size_t GetNumLiveAssets() const
    {
        return numAssets;
    }

    size_t GetNumAssets() const
    {
        return numAssets;
    }

    // 某個 asset 目前的消費方引用數。
    uint32_t GetAssetRefCount(const AssetId &id) const
    {
        return id ? id->refCount : 0;
    }

    // 某顆共享內容區塊目前被多少個「已登記的」asset 指向(#86:掃表推得,
    // 不再依賴 block 自己的 refCount)。
    uint32_t GetBlockRefCount(const BlockId &block)
    {
        if (!block)
            return 0;
        std::lock_guard<std::mutex> lock(blocksMutex);
        uint32_t count = 0;
        for (typename HashTable<uint64_t, AssetId>::Iterator it = assets.First(); it != assets.Last(); ++it)
            if (it->Value()->pBlock == block)
                count++;
        return count;
    }

    // 跨 key 共享的內容區塊數(不同 contentHash 的個數)。供測試驗證「相同內容只存一份」。
    // 註:直接由 numSharedBlocks 計數推得,而非讀 blocks.Length() —— HashTable 在
    // 擴張/收縮 rehash 時不會重置 nElements(既有 # 待修),Length() 會雙算。
    size_t GetNumSharedBlocks()
    {
        std::lock_guard<std::mutex> lock(blocksMutex);
        return numSharedBlocks;
    }

    // 目前仍 live 的共享內容區塊數(= GetNumSharedBlocks 的別名,語意上「尚未卸載的內容」)。
    size_t GetNumLiveBlocks()
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
        // #86:loaded/pBlock 由 worker 寫入 → 加鎖讀。
        std::lock_guard<std::mutex> lock(blocksMutex);
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
        // id 是 LoadAsync 才建的全新 Asset(loaded=false、pBlock 空 → 不需重設;
        // 移除舊版 unlocked 重設:它會與稍後的加鎖寫入打架)。

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
                        id->pBlock = existing; // content 去重:與既有 block 共用(跨 key 共享)
                        id->loaded = true;
                    }
                }
                if (!id->pBlock)
                {
                    id->pBlock = SharedPtr<AssetBlock>::Construct(rawBytes, contentHash);
                    blocks.Insert(contentHash, id->pBlock); // 新內容:登記進去重表
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

    // 真正卸載:從 assets 表移除(釋放 key)、歸還指向的共享 block。
    // block 的記憶體存亡全由 SharedPtr 計數決定(§5.3「最後一個 release 才真正 unload」);
    // 這裡只負責:表內不再有其他 asset 指向它時,把 block 從去重表移除。
    // #86:不再有 AssetBlock::refCount —— 「有沒有其他人指向」用掃表得出。
    // 呼叫端必須持有 id 的某份 strong ref(caller 的 SharedPtr 或 completedJobs)。
    void UnloadAsset(AssetId id)
    {
        if (!id)
            return;

        // 1) 移除 assets 表的 key(避免被 LoadAsync 去重命中、不再被 GetLoadedBytes 查得)
        typename HashTable<uint64_t, AssetId>::Iterator itr = assets.Find(id->keyHash);
        if (itr != assets.Last())
        {
            assets.Remove(id->keyHash);
            numAssets--;
        }

        // 2) 歸還持有的共享 block;最後一個「已登記」的 asset 離開 → 從去重表移除。
        //    (快照與清空都在 blocksMutex 內 —— worker 可能正在寫 id->pBlock。)
        {
            std::lock_guard<std::mutex> lock(blocksMutex);
            if (id->pBlock)
            {
                BlockId block = id->pBlock; // 先快照一份,再清掉本 asset 的引用
                id->pBlock = BlockId();      // 真正 unload bytes 由 SharedPtr 計數決定
                if (!HasSharedRefs(block))
                {
                    blocks.Remove(block->contentHash);
                    numSharedBlocks--;
                }
            }
        }

        id->released = true;
    }

    // blocksMutex 保護下呼叫:掃描已登記 asset,看是否仍有 asset 指向 block。
    bool HasSharedRefs(const BlockId &block)
    {
        for (typename HashTable<uint64_t, AssetId>::Iterator it = assets.First(); it != assets.Last(); ++it)
            if (it->Value()->pBlock == block)
                return true;
        return false;
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
    size_t numSharedBlocks = 0;                       // blocks 表的有效登錄數(見 GetNumSharedBlocks 註)
    std::mutex blocksMutex;
    DynamicArray<AssetId> completedJobs;
    size_t completedCursor = 0;
    std::mutex completedJobsMutex;
};

#endif // ASSETMANAGER_HPP