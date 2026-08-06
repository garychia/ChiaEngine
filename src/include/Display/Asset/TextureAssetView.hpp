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
// 把該 asset 的內容交給注入的 `OnTextureReady` 回呼(renderer 端:stbi 解碼 → GPU 上傳)。
//
// 非同步分割:檔案 I/O + 內容定址在 worker;decode→upload 在事件派發主執行緒
// (OnAssetLoaded 由 DispatchCompletedEvents 內觸發)。View 本身不碰 GPU API,
// 只做「完成事件 → 轉交 decode/upload」的中繼 → 可 headless 測試、renderer 無關。
class TextureAssetView
{
  public:
    // 注入的「renderer 端貼圖就緒處理器」:拿已載入資產的 RGBA bytes 上傳 GPU 貼圖。
    // 帶一個 userData(context)參數讓 renderer/測試掛自己的實例。
    typedef void (*TextureReadyHook)(const DynamicArray<unsigned char> &rgbaBytes, size_t contentHash,
                                     void *pUserData);

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

    // LoadedEvent 回調:資產完成載入 → 把 bytes 中繼給注入 hook。
    void OnAssetLoaded(SharedPtr<Asset> asset)
    {
        if (hook && asset && asset->loaded && asset->pBlock)
            hook(asset->pBlock->bytes, asset->ContentHash(), userData);
    }

    bool IsAttached() const
    {
        return pManager != nullptr;
    }

  private:
    AssetManager *pManager;
    TextureReadyHook hook;
    void *userData;
};

#endif // TEXTURE_ASSET_VIEW_HPP