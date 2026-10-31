#ifndef TEXTURE_ASSET_VIEW_TEST_HPP
#define TEXTURE_ASSET_VIEW_TEST_HPP

#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

#include "Test.hpp"
#include "System/Asset/AssetManager.hpp"
#include "System/Job/JobSystem.hpp"
#include "Display/Asset/TextureAssetView.hpp"

namespace textureviewtest
{
// 記錄 TextureAssetView 解碼後的中繼結果(透過 userData context)。
struct TextureViewObserver
{
    size_t onReadyCount = 0;
    size_t lastContentHash = 0;
    int lastWidth = 0;
    int lastHeight = 0;
    unsigned char lastRgba[4] = {0, 0, 0, 0};
    bool lastWasNull = false;

    static void OnTextureReady(const unsigned char *rgbaPixels, int width, int height,
                               size_t contentHash, void *pUserData)
    {
        TextureViewObserver *self = static_cast<TextureViewObserver *>(pUserData);
        self->onReadyCount++;
        self->lastContentHash = contentHash;
        self->lastWidth = width;
        self->lastHeight = height;
        self->lastWasNull = (rgbaPixels == nullptr);
        if (rgbaPixels)
        {
            self->lastRgba[0] = rgbaPixels[0];
            self->lastRgba[1] = rgbaPixels[1];
            self->lastRgba[2] = rgbaPixels[2];
            self->lastRgba[3] = rgbaPixels[3];
        }
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
        TEST_MESSAGE("TextureAssetView: typed LoadedEvent -> stb_image decode -> renderer upload hook");

        const char *pathA = TestTempFilePath("chia_view_a.tga");
        const char *pathB = TestTempFilePath("chia_view_b.tga");
        WriteTestTga(pathA, 1, 1, 10, 20, 30, 255); // RGBA(10,20,30,255)
        WriteTestTga(pathB, 2, 1, 0, 20, 40, 255);  // RGBA(0,20,40,255)

        JobSystem jobs(2);
        AssetManager manager(jobs);
        TextureAssetView view;

        // View 是 renderer 無關消費者:訂閱 AssetManager 的 typed LoadedEvent。
        view.SubscribeTo(manager);
        EXPECT_TRUE(view.IsAttached(), "TextureAssetView subscribes to AssetManager", true);

        TextureViewObserver obs;
        view.SetTextureReadyHook(&TextureViewObserver::OnTextureReady, &obs);

        // 發起 N 個載入 → 全部收到 LoadedEvent → View stb_image 解碼 → 轉交 renderer hook。
        SharedPtr<Asset> hA = manager.LoadAsync(pathA);
        SharedPtr<Asset> hB = manager.LoadAsync(pathB);
        jobs.WaitForIdle();
        const size_t nEvents = manager.DispatchCompletedEvents();

        EXPECT_TRUE(nEvents == 2, "async loads both complete -> 2 LoadedEvents", true);
        EXPECT_TRUE(hA->loaded && hB->loaded, "both asset bytes loaded", true);
        EXPECT_TRUE(obs.onReadyCount == 2, "TextureAssetView relayed every completed asset to renderer hook", true);

        // 最後一次事件 = B:寬 2、高 1、G=20、A=255、R+B=40(不依賴 stb_image 是否交換 R/B 通道)
        EXPECT_TRUE(obs.lastWidth == 2 && obs.lastHeight == 1, "decoded size = 2x1", true);
        EXPECT_TRUE(obs.lastRgba[1] == 20 && obs.lastRgba[3] == 255,
                    "decoded RGBA G=20, A=255", true);
        EXPECT_TRUE(obs.lastRgba[0] + obs.lastRgba[2] == 40,
                    "decoded RGBA R+B=40 (channel order agnostic)", true);

        // 解碼失敗案例:餵入非圖片資料 → hook 收到 nullptr
        const char *pathBad = TestTempFilePath("chia_view_bad.bin");
        WriteTestFile(pathBad, "NOT-AN-IMAGE");
        SharedPtr<Asset> hBad = manager.LoadAsync(pathBad);
        jobs.WaitForIdle();
        manager.DispatchCompletedEvents();
        EXPECT_TRUE(obs.lastWasNull, "undecodable bytes -> hook receives nullptr", true);

        // 綁定重複呼叫不重複訂閱(去重)。
        view.SubscribeTo(manager);
        view.Unsubscribe();
        EXPECT_TRUE(!view.IsAttached(), "TextureAssetView unsubscribes cleanly", true);

        std::remove(pathA);
        std::remove(pathB);
        std::remove(pathBad);

        SUCCESS_MESSAGE("TextureAssetViewTest");
        return true;
    }

  private:
    static void WriteTestFile(const char *path, const char *content)
    {
        std::ofstream file(path, std::ios::binary);
        file.write(content, static_cast<std::streamsize>(std::strlen(content)));
    }

    // 手工組裝最小 uncompressed TGA(image type 2, 32-bit, 無 RLE, 無顏色表)。
    // 像素以 BGRA 順序儲存(stb_image 讀取 req_comp=4 時會統一成 RGBA)。
    static void WriteTestTga(const char *path, int w, int h,
                             unsigned char r, unsigned char g, unsigned char b,
                             unsigned char a)
    {
        std::vector<unsigned char> tga;
        // 18-byte header
        tga.push_back(0x00);          // idlength
        tga.push_back(0x00);          // colormaptype
        tga.push_back(0x02);          // datatype = uncompressed true-color
        tga.push_back(0x00); tga.push_back(0x00); tga.push_back(0x00); tga.push_back(0x00); tga.push_back(0x00); // colormap spec (5 bytes)
        tga.push_back(0x00); tga.push_back(0x00);          // x_origin
        tga.push_back(0x00); tga.push_back(0x00);          // y_origin
        tga.push_back(static_cast<unsigned char>(w & 0xFF));
        tga.push_back(static_cast<unsigned char>((w >> 8) & 0xFF));
        tga.push_back(static_cast<unsigned char>(h & 0xFF));
        tga.push_back(static_cast<unsigned char>((h >> 8) & 0xFF));
        tga.push_back(0x20);          // pixel depth = 32
        tga.push_back(0x00);          // image descriptor
        // pixel data (BGRA), 重複 w*h 次
        for (int i = 0; i < w * h; ++i)
        {
            tga.push_back(b); tga.push_back(g); tga.push_back(r); tga.push_back(a);
        }

        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char *>(tga.data()),
                   static_cast<std::streamsize>(tga.size()));
    }
};

} // namespace textureviewtest

#endif // TEXTURE_ASSET_VIEW_TEST_HPP
