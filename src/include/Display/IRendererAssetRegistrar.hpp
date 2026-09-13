#ifndef IRENDERER_ASSET_REGISTRAR_HPP
#define IRENDERER_ASSET_REGISTRAR_HPP

// IRendererAssetRegistrar — 渲染資產(mesh 幾何 / material)的註冊 seam(issue #83)。
//
// #55 起,mesh/material 走「content-hash id」定址:View 側把 renderable 幾何登記成
// meshId、把 texture/MaterialSource 登記成 materialId,再由 Frame 的 DrawMesh 以
// id 參考。這些註冊必須落在執行 Frame 的那個後端(因為 GPU buffer 資源屬
// executor 擁有)。
//
// 為什麼要有這個獨立介面(而不是掛在 IFrameExecutor):
//   - 註冊是「以 content-hash 上傳資產」,與每一幀的 Execute 生命週期不同;
//   - #83 之前,RegisterMeshGeometry/RegisterMaterial 只存在於 concrete
//     VulkanRenderer,View 側被迫 dynamic_cast<VulkanRenderer*>(&renderer) — 而
//     renderer 實際是 Renderer facade,cast 永遠回 null,多材質示範悄悄失效。
//     此介面讓 View 只依賴「註冊能力」,而不是後端實類。
//
// 僅 VulkanExecutor(VulkanRenderer)實作;legacy 後端(DirectX/OpenGL 已 deprecated)
// 無此 seam。MaterialSource 是僅含 Texture* + inline RGBA 的純值型別,與具體
// 後端無關 → 定義在此(取代原本藏在 VulkanRenderer 內的巢狀 struct)。
//
// Build 端由 CMake 以 VULKAN_ENABLED gate 決定誰實作。

#include <cstdint>

class RenderInfo;
struct Texture;

// 材質來源:#55 的「第二 material 不需磁碟資產」約定。
//   pTexture 非 null → stbi 載入 imagePath(磁碟資產);
//   pRawRGBA 非 null → inline raw RGBA(width × height × 4)。
struct MaterialSource
{
    const Texture *pTexture = nullptr;
    const unsigned char *pRawRGBA = nullptr;
    uint32_t width = 0, height = 0;
};

class IRendererAssetRegistrar
{
  public:
    virtual ~IRendererAssetRegistrar() = default;

    // 註冊內容定址 geometry:renderable 的幾何(vertex/index/UV)上傳成 meshId 的
    // GPU buffer。成功回 true(meshId 之後可由 Frame::DrawMesh 繪製)。
    virtual bool RegisterMeshGeometry(uint64_t meshId, const RenderInfo &info) = 0;

    // 註冊 per-material texture:materialId → MaterialSource。
    // 成功後 Frame::BindMaterial(materialId) + DrawMesh 綁該材質的 descriptor set。
    virtual bool RegisterMaterial(uint64_t materialId, const MaterialSource &source) = 0;
};

#endif // IRENDERER_ASSET_REGISTRAR_HPP