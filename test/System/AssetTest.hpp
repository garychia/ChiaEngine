#ifndef ASSETTEST_HPP
#define ASSETTEST_HPP

#include <cstdio>
#include <cstring>
#include <fstream>

#include "Test.hpp"
#include "System/Asset/AssetManager.hpp"
#include "System/Job/JobSystem.hpp"

namespace assettest
{
// 訂閱 LoadedEvent 的測試觀察者。
class AssetObserver
{
  public:
    DynamicArray<String> loadedKeys;
    size_t eventCount = 0;

    void OnLoaded(SharedPtr<Asset> asset)
    {
        loadedKeys.Append(asset->key);
        eventCount++;
    }
};

class AssetTest : public Test
{
  public:
    AssetTest() : Test("AssetTest")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("Asset async load + dedup + content-addressed byte sharing");

        const char *pathA = "/tmp/chia_asset_a.bin";
        const char *pathB = "/tmp/chia_asset_b.bin";
        const char *pathCopy = "/tmp/chia_asset_a_copy.bin"; // 內容與 A 相同,不同 key
        WriteTestFile(pathA, "AAA");
        WriteTestFile(pathB, "BBBB");
        WriteTestFile(pathCopy, "AAA"); // byte-identical to A

        JobSystem jobs(2);
        AssetManager manager(jobs);
        AssetObserver observer;
        manager.LoadedEvent.Subscribe(&observer, &AssetObserver::OnLoaded);

        // 非同步請求:A、B、再 A(重複 → 依 key 去重)
        SharedPtr<Asset> hA = manager.LoadAsync(pathA);
        SharedPtr<Asset> hB = manager.LoadAsync(pathB);
        SharedPtr<Asset> hA2 = manager.LoadAsync(pathA);

        // 立即:還沒載入(真非同步 — worker 尚未或有機會跑)
        EXPECT_TRUE(!hA->loaded, "async: A not loaded immediately", true);

        // 等 worker 全部完成 → 主執行緒派發完成事件
        jobs.WaitForIdle();
        const size_t nEvents = manager.DispatchCompletedEvents();

        EXPECT_TRUE(hA->loaded && hB->loaded, "assets load after workers finish", true);
        EXPECT_TRUE(hA2 == hA, "duplicate request returns the same handle (key-dedup)", true);
        EXPECT_TRUE(manager.GetNumAssets() == 2, "key-dedup registers exactly 2 assets", true);
        EXPECT_TRUE(nEvents == 2, "exactly 2 completion events (key-dedup does not re-fire)", true);
        EXPECT_TRUE(observer.eventCount == 2, "observer saw 2 events", true);
        EXPECT_TRUE(observer.loadedKeys.GetNElements() == 2, "observer collected 2 keys", true);

        // 內容與指紋正確(新 API:Bytes() / ContentHash())
        EXPECT_TRUE(hA->Bytes().Length() == 3 && hB->Bytes().Length() == 4, "byte sizes match file sizes", true);
        EXPECT_TRUE(hA->ContentHash() == FNV1A64(FNV1A64_OFFSET, "AAA", 3), "A content hash == FNV(bytes)", true);
        EXPECT_TRUE(hB->ContentHash() == FNV1A64(FNV1A64_OFFSET, "BBBB", 4), "B content hash == FNV(bytes)", true);

        // GetLoadedBytes 依 key 回拉
        DynamicArray<unsigned char> fetched;
        EXPECT_TRUE(manager.GetLoadedBytes(pathA, &fetched), "GetLoadedBytes finds A", true);
        EXPECT_TRUE(fetched.Length() == 3 && fetched[0] == 'A', "GetLoadedBytes A content ok", true);

        // ── P5 part 2:跨 key 內容共享 ──
        // 再載入一個 byte-identical 到 A 的不同 key。完成後:
        //   * 應命中 A 的共享 block(同一個 AssetBlock 指標,非複製)
        //   * numAssets 增加(不同 key),但 numSharedBlocks 仍只 3 種內容(A/B)… 正確應是:
        //     A=AAA,B=BBBB,copy=AAA → 不同內容只有 2 種,共享 block 數 = 2
        SharedPtr<Asset> hCopy = manager.LoadAsync(pathCopy);
        jobs.WaitForIdle();
        const size_t nEvents2 = manager.DispatchCompletedEvents();

        EXPECT_TRUE(hCopy->loaded, "copy asset loaded", true);
        EXPECT_TRUE(hCopy->pBlock == hA->pBlock, "identical content shares the same AssetBlock (pointer-equal)", true);
        EXPECT_TRUE(manager.GetNumAssets() == 3, "3 distinct keys registered", true);
        EXPECT_TRUE(manager.GetNumSharedBlocks() == 2, "only 2 unique contents → 2 shared blocks", true);
        EXPECT_TRUE(nEvents2 == 1, "copy fired exactly 1 completion event", true);
        EXPECT_TRUE(observer.eventCount == 3, "observer saw 3 total events", true);

        // ── P7c:引用計數(§5.3)──
        // 內容層:兩顆不同 key 指向同一 content → block refCount 應為 2。
        EXPECT_TRUE(manager.GetBlockRefCount(hA->pBlock) == 2, "shared block refcount == 2 (A + copy)", true);
        // key 層:每個 LoadAsync 是一個消費方引用。
        EXPECT_TRUE(manager.GetAssetRefCount(hA) == 2, "A requested twice → asset refcount == 2", true);
        EXPECT_TRUE(manager.GetAssetRefCount(hCopy) == 1, "copy requested once → asset refcount == 1", true);

        // 釋放 hCopy:block 還剩 A 指向 → 仍 live,refCount 降為 1。
        manager.Release(hCopy);
        EXPECT_TRUE(manager.GetBlockRefCount(hA->pBlock) == 1, "after releasing copy, block refcount == 1", true);
        EXPECT_TRUE(manager.GetNumSharedBlocks() == 2, "block still alive (A keeps it)", true);
        EXPECT_TRUE(manager.GetNumLiveAssets() == 2, "copy asset evicted but A/B still live", true);

        // 釋放 hA 一次(refCount 2 → 1):A 仍在、block 仍在。
        manager.Release(hA);
        EXPECT_TRUE(manager.GetBlockRefCount(hA->pBlock) == 1, "A still references block", true);

        // 釋放 hA2(hA 的第二次引用 → 0):A 卸載,且是最後一個指向 block 的 asset → block unload。
        manager.Release(hA2);
        EXPECT_TRUE(manager.GetNumSharedBlocks() == 1, "last release of A unloads its block (only B remains)", true);
        EXPECT_TRUE(manager.GetNumLiveAssets() == 1, "only B remains live", true);

        // 非同步完成前就被釋放:不發事件、完成時直接 evict。
        {
            JobSystem jobsB(1);
            AssetManager managerB(jobsB);
            AssetObserver obsB;
            managerB.LoadedEvent.Subscribe(&obsB, &AssetObserver::OnLoaded);
            SharedPtr<Asset> hEarly = managerB.LoadAsync(pathB);
            managerB.Release(hEarly); // 載入完成前就釋放
            jobsB.WaitForIdle();
            const size_t nEarly = managerB.DispatchCompletedEvents();
            EXPECT_TRUE(nEarly == 0, "released-before-load asset fires no event", true);
            EXPECT_TRUE(obsB.eventCount == 0, "observer saw no event for early-released asset", true);
            EXPECT_TRUE(managerB.GetNumLiveAssets() == 0, "early-released asset evicted at dispatch", true);
            EXPECT_TRUE(managerB.GetNumSharedBlocks() == 0, "early-released content not retained", true);
            managerB.LoadedEvent.Unsubscribe(&obsB);
        }

        manager.LoadedEvent.Unsubscribe(&observer);
        std::remove(pathA);
        std::remove(pathB);
        std::remove(pathCopy);

        SUCCESS_MESSAGE("AssetTest");
        return true;
    }

  private:
    static void WriteTestFile(const char *path, const char *content)
    {
        std::ofstream file(path, std::ios::binary);
        file.write(content, static_cast<std::streamsize>(std::strlen(content)));
    }
};
} // namespace assettest

#endif // ASSETTEST_HPP