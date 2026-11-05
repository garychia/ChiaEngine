#ifndef MATERIAL_COMPONENT_HPP
#define MATERIAL_COMPONENT_HPP

#include <cstdint>

#include "Geometry/3D/Point3D.hpp"

// 節點材質綁定:materialId = 0 表示未綁定(無貼圖/預設材質)。materialId 與
// IRendererAssetRegistrar::RegisterMaterial 的 content-hash id 相同,由
// EditorSession 經 AssetManager→TextureAssetView 解碼後註冊產生。
//
// PBR 欄位(A2.1):albedo 基礎顏色、metallic 金屬度、roughness 粗糙度、
// normalMapId 法線貼圖 content-hash id(0 表示無法線貼圖)。預設為白色
// 絕緣體(albedo=1, metallic=0, roughness=1)。
struct MaterialComponent
{
    uint64_t materialId = 0;
    Point3D albedo = Point3D(1, 1, 1);
    float metallic = 0.0f;
    float roughness = 1.0f;
    uint64_t normalMapId = 0;

    MaterialComponent() = default;
    explicit MaterialComponent(uint64_t materialId)
        : materialId(materialId), albedo(1, 1, 1), metallic(0.0f), roughness(1.0f), normalMapId(0)
    {
    }
};

#endif // MATERIAL_COMPONENT_HPP
