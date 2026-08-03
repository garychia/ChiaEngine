#ifndef ASSETMANAGER_HPP
#define ASSETMANAGER_HPP

#include <cstddef>
#include <cstdint>
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

// 資產 v1:整包位元組 + 內容指紋。之後 Texture/Shader 直接吃 Asset.bytes。
struct Asset
{
    String key;
    uint64_t keyHash;     // key 的 FNV-1a(HashTable 只支援整數 key → 內容定址)
    DynamicArray<unsigned char> bytes;
    uint64_t contentHash; // 內容指紋(FNV-1a over bytes)
    bool loaded;

    Asset(const String &key, uint64_t keyHash) : key(key), keyHash(keyHash), contentHash(0), loaded(false)
    {
    }
};

// AssetManager:HashTable<uint64_t, AssetHandle>(key = key 的 FNV-1a,內容定址)
// + 按 key 去重;非同步載入完成事件由主執行緒統一廣播(Event 非 thread-safe)。
//
// 使用慣例:
//  - 所有公開方法在「主執行緒」呼叫(LoadAsync / DispatchCompletedEvents)。
//  - 載入在 JobSystem 的 worker 執行,只碰 Asset.bytes,不碰 assets 表。
//  - 完成事件不是從 worker 直接發 — 而是塞進 completed 佇列,
//    由 DispatchCompletedEvents() 在主執行緒成批廣播 LoadedEvent。
class AssetManager
{
  public:
    typedef SharedPtr<Asset> AssetHandle;

    explicit AssetManager(JobSystem &jobs) : jobs(jobs), numAssets(0)
    {
    }

    // 完成事件(主執行緒,DispatchCompletedEvents 內觸發)。
    Event<void(AssetHandle)> LoadedEvent;

    // 非同步請求載入:已載入/載入中 → 回傳既有 handle(去重,不重複讀檔、不重複發事件)。
    AssetHandle LoadAsync(const String &key)
    {
        const uint64_t keyHash = HashKey(key);
        typename HashTable<uint64_t, AssetHandle>::Iterator itr = assets.Find(keyHash);
        if (itr != assets.Last())
            return itr->Value();

        AssetHandle handle = SharedPtr<Asset>::Construct(key, keyHash);
        assets.Insert(keyHash, handle);
        numAssets++;

        // worker 只碰 handle->bytes;Asset 由 SharedPtr 持有跨執行緒安全。
        jobs.Enqueue([this, handle] { LoadWorker(handle); });
        return handle;
    }

    // 主執行緒:把完成佇列裡的資產依序廣播 LoadedEvent。回傳本批事件數。
    size_t DispatchCompletedEvents()
    {
        size_t n = 0;
        for (;;)
        {
            AssetHandle handle;
            {
                std::lock_guard<std::mutex> lock(completedMutex);
                if (completedCursor >= completed.Length())
                    break;
                handle = completed[completedCursor];
                completedCursor++;
            }
            LoadedEvent.Invoke(handle);
            n++;
        }
        // 捨棄已讀的 slot(避免佇列無限長)
        {
            std::lock_guard<std::mutex> lock(completedMutex);
            if (completedCursor == completed.Length())
            {
                completed.RemoveAll();
                completedCursor = 0;
            }
        }
        return n;
    }

    size_t GetNumAssets() const
    {
        return numAssets;
    }

    // 已完成且已派發的資產的資料(依 key 查)。找不到回傳 false。
    bool GetLoadedBytes(const String &key, DynamicArray<unsigned char> *pOut)
    {
        typename HashTable<uint64_t, AssetHandle>::Iterator it = assets.Find(HashKey(key));
        if (it == assets.Last())
            return false;
        AssetHandle handle = it->Value();
        if (!handle->loaded)
            return false;
        *pOut = handle->bytes;
        return true;
    }

  private:
    static uint64_t HashKey(const String &key)
    {
        const Str<char> utf8 = key.ToUTF8();
        return FNV1A64(FNV1A64_OFFSET, utf8.CStr(), utf8.Length());
    }

    void LoadWorker(AssetHandle handle)
    {
        handle->loaded = false;

        const Str<char> utf8Path = handle->key.ToUTF8();
        std::ifstream file(utf8Path.CStr(), std::ios::binary);
        if (file)
        {
            file.seekg(0, std::ios::end);
            const std::streamoff size = file.tellg();
            file.seekg(0, std::ios::beg);
            const size_t n = size > 0 ? static_cast<size_t>(size) : 0;
            handle->bytes.Resize(n);
            if (n > 0)
                file.read(reinterpret_cast<char *>(&handle->bytes[0]), static_cast<std::streamsize>(n));
            // 內容指紋;空檔件(0 size)特例避免 &[0] UB。
            handle->contentHash = (n > 0) ? FNV1A64(FNV1A64_OFFSET, &handle->bytes[0], n) : FNV1A64_OFFSET;
            handle->loaded = true;
        }

        {
            std::lock_guard<std::mutex> lock(completedMutex);
            completed.Append(handle);
        }
    }

    JobSystem &jobs;
    HashTable<uint64_t, AssetHandle> assets;
    size_t numAssets;
    DynamicArray<AssetHandle> completed;
    size_t completedCursor = 0;
    std::mutex completedMutex;
};

#endif // ASSETMANAGER_HPP