#ifndef MATERIAL_BINDING_TEST_HPP
#define MATERIAL_BINDING_TEST_HPP

#include "Data/String.hpp"
#include "Display/GUI/EditorSession.hpp"
#include "Display/GUI/InspectorLayer.hpp"
#include "Geometry/2D/Point2D.hpp"
#include "Scene/MaterialComponent.hpp"
#include "Scene/SceneSystem.hpp"
#include "System/World/Entity.hpp"
#include "Test.hpp"

#include <cstdint>

// Texture/materialId 綁定到選中場景節點(#168 優先級 2)。
// 1. SceneSystem SetMaterial/GetMaterial:綁定/查詢節點 materialId。
// 2. EditorSession BindTextureToSelectedEntity/GetSelectedEntityMaterialId:
//    將已載入貼圖的 materialId 綁定到選中節點。
// 3. InspectorLayer 材質標籤:顯示 "Material: <id>" / "Material: None"。
// 全 headless,無需實際貼圖檔(未載入時 materialId = 0)。
namespace materialbinding_test
{

class MaterialBindingTest : public Test
{
  public:
    MaterialBindingTest() : Test("MaterialBinding")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:SceneSystem SetMaterial/GetMaterial ----
        {
            SceneSystem system;
            Entity root = system.CreateNode();

            // 初始未綁定 → materialId = 0
            uint64_t matId = 0;
            EXPECT_TRUE(system.GetMaterial(root.GetIndex(), matId), "GetMaterial 回傳 true", true);
            EXPECT_TRUE(matId == 0, "初始 materialId 為 0", true);

            // 綁定 materialId = 42
            system.SetMaterial(root.GetIndex(), 42);
            matId = 0;
            EXPECT_TRUE(system.GetMaterial(root.GetIndex(), matId), "GetMaterial 回傳 true", true);
            EXPECT_TRUE(matId == 42, "SetMaterial 後 materialId == 42", true);

            // 解綁(materialId = 0)
            system.SetMaterial(root.GetIndex(), 0);
            matId = 999;
            EXPECT_TRUE(system.GetMaterial(root.GetIndex(), matId), "GetMaterial 回傳 true", true);
            EXPECT_TRUE(matId == 0, "解綁後 materialId 回到 0", true);

            // 不存在的節點 → GetMaterial 回 false
            EXPECT_TRUE(!system.GetMaterial(999999, matId), "不存在節點 GetMaterial 回 false", true);
        }

        // ---- AC2:EditorSession BindTextureToSelectedEntity(無載入貼圖 → materialId 0) ----
        {
            SceneSystem system;
            Entity root = system.CreateNode();
            EditorSession session(&system, nullptr, nullptr);
            session.Select(root.GetIndex());

            // 未載入任何貼圖 → 綁定任何路徑都回 materialId 0(無效)
            session.BindTextureToSelectedEntity(String(u"assets/textures/nonexistent.ppm"));
            EXPECT_TRUE(session.GetSelectedEntityMaterialId() == 0,
                        "未載入貼圖時綁定 → materialId 為 0", true);
        }

        // ---- AC3:InspectorLayer 材質標籤(無 pSession → "Material: None") ----
        {
            SceneSystem system;
            UndoStack stack;
            Entity root = system.CreateNode();

            InspectorLayer inspector(String(u"Inspector"), Point2D(1000.f, 800.f),
                                     Border(800.f, 30.f, 200.f, 400.f), PanelDock::RightDock,
                                     &system, &stack, nullptr);
            Selection sel;
            sel.entityIndex = root.GetIndex();
            sel.hasSelection = true;
            inspector.SetSelection(&sel);
            inspector.Update();

            EXPECT_TRUE(inspector.GetMaterialLabel() == String(u"Material: None"),
                        "無 pSession → 標籤顯示 Material: None", true);
        }

        // ---- AC4:EditorSession GetTextureCount/GetTexturePathAt(無貼圖 → 0/空) ----
        {
            SceneSystem system;
            EditorSession session(&system, nullptr, nullptr);
            EXPECT_TRUE(session.GetTextureCount() == 0, "無載入貼圖 → GetTextureCount 為 0", true);
            EXPECT_TRUE(session.GetTexturePathAt(0).Length() == 0,
                        "越界/無貼圖 → GetTexturePathAt 回空字串", true);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }
};

} // namespace materialbinding_test

#endif // MATERIAL_BINDING_TEST_HPP
