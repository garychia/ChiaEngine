#ifndef MATERIAL_COMPONENT_HPP
#define MATERIAL_COMPONENT_HPP

#include <cstdint>

// 節點材質綁定:materialId = 0 表示未綁定(無貼圖/預設材質)。materialId 與
// IRendererAssetRegistrar::RegisterMaterial 的 content-hash id 相同,由
// EditorSession 經 AssetManager→TextureAssetView 解碼後註冊產生。
struct MaterialComponent
{
    uint64_t materialId = 0;

    MaterialComponent() = default;
    explicit MaterialComponent(uint64_t materialId) : materialId(materialId) {}
};

#endif // MATERIAL_COMPONENT_HPP
