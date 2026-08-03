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
        TEST_MESSAGE("Asset async load + dedup + completion event");

        const char *pathA = "/tmp/chia_asset_a.bin";
        const char *pathB = "/tmp/chia_asset_b.bin";
        WriteTestFile(pathA, "AAA");
        WriteTestFile(pathB, "BBBB");

        JobSystem jobs(2);
        AssetManager manager(jobs);
        AssetObserver observer;
        manager.LoadedEvent.Subscribe(&observer, &AssetObserver::OnLoaded);

        // 非同步請求:A、B、再 A(重複 → 應去重)
        SharedPtr<Asset> hA = manager.LoadAsync(pathA);
        SharedPtr<Asset> hB = manager.LoadAsync(pathB);
        SharedPtr<Asset> hA2 = manager.LoadAsync(pathA);

        // 立即:還沒載入(真非同步 — worker 尚未或有機會跑)
        EXPECT_TRUE(!hA->loaded, "async: A not loaded immediately", true);

        // 等 worker 全部完成 → 主執行緒派發完成事件
        jobs.WaitForIdle();
        const size_t nEvents = manager.DispatchCompletedEvents();

        EXPECT_TRUE(hA->loaded && hB->loaded, "assets load after workers finish", true);
        EXPECT_TRUE(hA2 == hA, "duplicate request returns the same handle (dedup)", true);
        EXPECT_TRUE(manager.GetNumAssets() == 2, "dedup registers exactly 2 assets", true);
        EXPECT_TRUE(nEvents == 2, "exactly 2 completion events (dedup does not re-fire)", true);
        EXPECT_TRUE(observer.eventCount == 2, "observer saw 2 events", true);
        EXPECT_TRUE(observer.loadedKeys.GetNElements() == 2, "observer collected 2 keys", true);

        // 內容與指紋正確
        EXPECT_TRUE(hA->bytes.Length() == 3 && hB->bytes.Length() == 4, "byte sizes match file sizes", true);
        EXPECT_TRUE(hA->contentHash == FNV1A64(FNV1A64_OFFSET, "AAA", 3), "A content hash == FNV(bytes)", true);
        EXPECT_TRUE(hB->contentHash == FNV1A64(FNV1A64_OFFSET, "BBBB", 4), "B content hash == FNV(bytes)", true);

        // GetLoadedBytes 依 key 回拉
        DynamicArray<unsigned char> fetched;
        EXPECT_TRUE(manager.GetLoadedBytes(pathA, &fetched), "GetLoadedBytes finds A", true);
        EXPECT_TRUE(fetched.Length() == 3 && fetched[0] == 'A', "GetLoadedBytes A content ok", true);

        manager.LoadedEvent.Unsubscribe(&observer);
        std::remove(pathA);
        std::remove(pathB);

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