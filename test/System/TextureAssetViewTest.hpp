#ifndef TEXTURE_ASSET_VIEW_TEST_HPP
#define TEXTURE_ASSET_VIEW_TEST_HPP

#include <cstdio>
#include <cstring>
#include <fstream>

#include "Test.hpp"
#include "System/Asset/AssetManager.hpp"
#include "System/Job/JobSystem.hpp"
#include "Display/Asset/TextureAssetView.hpp"

namespace textureviewtest
{
// 記錄 TextureAssetView 中繼結果的觀察者(透過 userData context)。
struct TextureViewObserver
{
    size_t onReadyCount = 0;
    size_t lastContentHash = 0;
    size_t lastByteLen = 0;

    static void OnTextureReady(const DynamicArray<unsigned char> &rgbaBytes, size_t contentHash, void *pUserData)
    {
        TextureViewObserver *self = static_cast<TextureViewObserver *>(pUserData);
        self->onReadyCount++;
        self->lastContentHash = contentHash;
        self->lastByteLen = rgbaBytes.Length();
    }
};

class TextureAssetViewTest : public Test
{
  public:
    TextureAssetViewTest() : Test("TextureAssetViewTest")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("TextureAssetView: typed LoadedEvent -> renderer decode/upload bridge (principle #4)");

        const char *pathA = "/tmp/chia_view_a.bin";
        const char *pathB = "/tmp/chia_view_b.bin";
        WriteTestFile(pathA, "TEX-A-BYTES");
        WriteTestFile(pathB, "TEX-B-BYTES");

        JobSystem jobs(2);
        AssetManager manager(jobs);
        TextureAssetView view;

        // View 是 renderer 無關消費者:訂閱 AssetManager 的 typed LoadedEvent。
        view.SubscribeTo(manager);
        EXPECT_TRUE(view.IsAttached(), "TextureAssetView subscribes to AssetManager", true);

        TextureViewObserver obs;
        view.SetTextureReadyHook(&TextureViewObserver::OnTextureReady, &obs);

        // 發起 N 個載入 → 全部收到 LoadedEvent → View 轉交 hook 給 renderer。
        SharedPtr<Asset> hA = manager.LoadAsync(pathA);
        SharedPtr<Asset> hB = manager.LoadAsync(pathB);
        jobs.WaitForIdle();
        const size_t nEvents = manager.DispatchCompletedEvents();

        EXPECT_TRUE(nEvents == 2, "async loads both complete -> 2 LoadedEvents", true);
        EXPECT_TRUE(hA->loaded && hB->loaded, "both asset bytes loaded", true);
        EXPECT_TRUE(obs.onReadyCount == 2, "TextureAssetView relayed every completed asset to renderer hook", true);
        EXPECT_TRUE(obs.lastByteLen == std::strlen("TEX-B-BYTES"),
                    "hook received the asset bytes (last = B)", true);

        // 綁定重複呼叫不重複訂閱(去重)。
        view.SubscribeTo(manager);
        view.Unsubscribe();
        EXPECT_TRUE(!view.IsAttached(), "TextureAssetView unsubscribes cleanly", true);

        std::remove(pathA);
        std::remove(pathB);

        SUCCESS_MESSAGE("TextureAssetViewTest");
        return true;
    }

  private:
    static void WriteTestFile(const char *path, const char *content)
    {
        std::ofstream file(path, std::ios::binary);
        file.write(content, static_cast<std::streamsize>(std::strlen(content)));
    }
};
} // namespace textureviewtest

#endif // TEXTURE_ASSET_VIEW_TEST_HPP