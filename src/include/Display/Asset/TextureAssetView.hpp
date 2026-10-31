#ifndef TEXTURE_ASSET_VIEW_HPP
#define TEXTURE_ASSET_VIEW_HPP

#include "Data/DynamicArray.hpp"
#include "Data/Pointers.hpp"
#include "System/Asset/AssetManager.hpp"
#include "System/Operation/Event.hpp"

// ── P7c:View 層的 typed 資產消費者(principle #4 — module 以 typed event 溝通)──
//
// 把「AssetManager(基礎層,載入原始 bytes)」與「renderer 視覺 projection」的耦合
// 拆開:TextureAssetView 訂閱 AssetManager::LoadedEvent(AssetId),資產載入完成後
// 用 stb_image 從原始 bytes 解碼成 RGBA 像素,再轉交給注入的 OnTextureReady
// 回呼(renderer 端:GPU 上傳,如 VulkanRenderer::RegisterMaterial 的 pRawRGBA 路徑)。
//
// 非同步分割:檔案 I/O + 內容定址在 worker;decode→upload 在事件派發主執行緒
// (OnAssetLoaded 由 DispatchCompletedEvents 內觸發)。View 本身不碰 GPU API,
// 只做「完成事件 → 解碼 → 轉交 decode/upload」的中繼 → 可 headless 測試、renderer 無關。
//
// 連線範例(View → renderer):
//   class TextureUploader {
//   public:
//     TextureUploader(IRendererAssetRegistrar &reg) : registrar(reg) {}
//     static void OnTextureReady(const unsigned char *rgba, int w, int h, size_t hash, void *pCtx)
//     {
//         auto self = static_cast<TextureUploader *>(pCtx);
//         MaterialSource src{}; src.pRawRGBA = rgba; src.width = w; src.height = h;
//         self->registrar.RegisterMaterial(hash, src); // content-hash 作為 materialId
//     }
//   private:
//     IRendererAssetRegistrar &registrar;
//   };
//   TextureAssetView view; TextureUploader uploader(*renderer);
//   view.SetTextureReadyHook(&TextureUploader::OnTextureReady, &uploader);
//   view.SubscribeTo(assetManager);
class TextureAssetView
{
  public:
    // 注入的「renderer 端貼圖就緒處理器」:拿 stb_image 解碼後的 RGBA pixels
    // 上傳 GPU 貼圖。rgbaPixels 在 hook 呼叫期間有效(TextureAssetView 持有),
    // hook 回傳後 renderer 應已自行拷貝(如 RegisterMaterial 寫入 VkImage)。
    // 解碼失敗時 rgbaPixels = nullptr, w = h = 0, contentHash 仍回報。
    typedef void (*TextureReadyHook)(const unsigned char *rgbaPixels, int width, int height,
                                     size_t contentHash, void *pUserData);

    TextureAssetView() : pManager(nullptr), hook(nullptr), userData(nullptr)
    {
    }

    // 綁定 AssetManager 並訂閱其 LoadedEvent(principle #4 typed event)。
    void SubscribeTo(AssetManager &manager)
    {
        if (pManager == &manager)
            return; // 已訂閱同一顆,避免重複
        if (pManager)
            Unsubscribe();
        pManager = &manager;
        pManager->LoadedEvent.Subscribe(this, &TextureAssetView::OnAssetLoaded);
    }

    void Unsubscribe()
    {
        if (pManager)
        {
            pManager->LoadedEvent.Unsubscribe(this);
            pManager = nullptr;
        }
    }

    // renderer 注入的 decode/upload 處理器(+ context)。
    void SetTextureReadyHook(TextureReadyHook cb, void *pCtx = nullptr)
    {
        hook = cb;
        userData = pCtx;
    }

    // LoadedEvent 回調:資產完成載入 → stb_image 解碼 → 中繼給注入 hook。
    void OnAssetLoaded(SharedPtr<Asset> asset);

    bool IsAttached() const
    {
        return pManager != nullptr;
    }

  private:
    AssetManager *pManager;
    TextureReadyHook hook;
    void *userData;
};

// 從記憶體 buffer 用 stb_image 解碼成 RGBA(4 通道,未乘积 alpha)。
// 成功回 true,outRGBA 持有 w*h*4 像素;失敗回 false,outRGBA 不動。
bool DecodeImageFromMemory(const unsigned char *buffer, size_t len, int *outW, int *outH,
                           DynamicArray<unsigned char> *outRGBA);

#endif // TEXTURE_ASSET_VIEW_HPP
