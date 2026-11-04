#ifndef MATERIAL_PBR_TEST_HPP
#define MATERIAL_PBR_TEST_HPP

#include "Data/String.hpp"
#include "Display/GUI/EditorSession.hpp"
#include "Scene/SceneSystem.hpp"
#include "Test.hpp"

#include <cstdio>
#include <fstream>

namespace materialpbr_test
{

class MaterialPBRTest : public Test
{
  public:
    MaterialPBRTest() : Test("MaterialPBR")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:MaterialComponent 預設 PBR 欄位 ----
        {
            MaterialComponent mat;
            EXPECT_TRUE(mat.materialId == 0, "預設 materialId=0", true);
            EXPECT_TRUE(mat.albedo.x == 1.f && mat.albedo.y == 1.f && mat.albedo.z == 1.f,
                        "預設 albedo=(1,1,1)", true);
            EXPECT_TRUE(mat.metallic == 0.f, "預設 metallic=0", true);
            EXPECT_TRUE(mat.roughness == 1.f, "預設 roughness=1", true);
            EXPECT_TRUE(mat.normalMapId == 0, "預設 normalMapId=0", true);
        }

        // ---- AC2:SceneSystem::SetPBR/GetPBR 往返 ----
        {
            SceneSystem system;
            Entity root = system.CreateNode();

            Point3D albedo(0.5f, 0.25f, 0.75f);
            system.SetPBR(root.GetIndex(), albedo, 0.8f, 0.3f);

            Point3D outAlbedo;
            float outMetallic = 0.f, outRoughness = 0.f;
            EXPECT_TRUE(system.GetPBR(root.GetIndex(), outAlbedo, outMetallic, outRoughness),
                        "GetPBR 回傳 true", true);
            EXPECT_TRUE(outAlbedo.x == 0.5f && outAlbedo.y == 0.25f && outAlbedo.z == 0.75f,
                        "albedo 往返正確", true);
            EXPECT_TRUE(outMetallic == 0.8f, "metallic 往返正確", true);
            EXPECT_TRUE(outRoughness == 0.3f, "roughness 往返正確", true);
        }

        // ---- AC3:EditorSession::SetSelectedEntityPBR 透過 UndoStack ----
        {
            SceneSystem system;
            EditorSession session(&system, nullptr, nullptr);

            Entity root = system.CreateNode();
            session.Select(root.GetIndex());

            session.SetSelectedEntityPBR(Point3D(0.1f, 0.2f, 0.3f), 0.5f, 0.6f);

            // UndoStack 應有一個 SetPBRCommand
            EXPECT_TRUE(session.GetUndoStack().GetUndoCount() == 1,
                        "UndoStack 有 1 個 SetPBRCommand", true);

            //  undo 應該回復到預設值
            Point3D outAlbedo;
            float outMetallic = 0.f, outRoughness = 0.f;
            session.GetUndoStack().Undo();
            EXPECT_TRUE(system.GetPBR(root.GetIndex(), outAlbedo, outMetallic, outRoughness),
                        "Undo 後 GetPBR 回傳 true", true);
            EXPECT_TRUE(outAlbedo.x == 1.f && outAlbedo.y == 1.f && outAlbedo.z == 1.f,
                        "Undo 後 albedo 回復預設 (1,1,1)", true);
            EXPECT_TRUE(outMetallic == 0.f, "Undo 後 metallic 回復 0", true);
            EXPECT_TRUE(outRoughness == 1.f, "Undo 後 roughness 回復 1", true);

            // redo 應該回到設定值
            session.GetUndoStack().Redo();
            EXPECT_TRUE(system.GetPBR(root.GetIndex(), outAlbedo, outMetallic, outRoughness),
                        "Redo 後 GetPBR 回傳 true", true);
            EXPECT_TRUE(outAlbedo.x == 0.1f && outAlbedo.y == 0.2f && outAlbedo.z == 0.3f,
                        "Redo 後 albedo=(0.1,0.2,0.3)", true);
            EXPECT_TRUE(outMetallic == 0.5f, "Redo 後 metallic=0.5", true);
            EXPECT_TRUE(outRoughness == 0.6f, "Redo 後 roughness=0.6", true);
        }

        // ---- AC4:Prefab 儲存/載入 PBR 欄位保持不變 ----
        {
            SceneSystem system;
            Entity root = system.CreateNode();
            system.SetPBR(root.GetIndex(), Point3D(0.9f, 0.1f, 0.4f), 0.7f, 0.2f);
            system.SetMaterial(root.GetIndex(), 42);

            Entity child = system.CreateNode(root);
            system.SetPBR(child.GetIndex(), Point3D(0.3f, 0.6f, 0.9f), 0.1f, 0.9f);

            const char *path = TestTempFilePath("chia_material_pbr_test.prefab");
            EXPECT_TRUE(system.SavePrefab(root, path), "SavePrefab 回傳 true", true);

            // 先嘗試手寫 v2 檔,驗證 ParsePrefabNodeLine
            const char *manualPath = TestTempFilePath("chia_material_pbr_manual.prefab");
            {
                std::ofstream manualFile(manualPath);
                manualFile << "prefab v2\n";
                // root: localIndex 0, parent -1, materialId 42, albedo(0.9,0.1,0.4), metallic 0.7, roughness 0.2, normalMapId 0, name "Root", identity TRS
                manualFile << "node 0 -1 42 0.9 0.1 0.4 0.7 0.2 0 Root 0 0 0 0 0 0 1 1 1\n";
                // child: localIndex 1, parent 0, materialId 0, albedo(0.3,0.6,0.9), metallic 0.1, roughness 0.9, normalMapId 0, name "Child", identity TRS
                manualFile << "node 1 0 0 0.3 0.6 0.9 0.1 0.9 0 Child 0 0 0 0 0 0 1 1 1\n";
                manualFile << "end\n";
            }

            // 清空場景
            system.DestroyNode(root);
            EXPECT_TRUE(!system.Alive(root), "DestroyNode 後 root 失效", true);

            // 載入手寫 v2 prefab
            Entity manualRoot = system.LoadPrefab(Entity(), manualPath);
            EXPECT_TRUE(manualRoot.GetRaw() != 0, "手寫 v2 prefab LoadPrefab 建立新根節點", true);

            Point3D outAlbedo;
            float outMetallic = 0.f, outRoughness = 0.f;
            EXPECT_TRUE(system.GetPBR(manualRoot.GetIndex(), outAlbedo, outMetallic, outRoughness),
                        "手寫 v2 載入後 GetPBR 回傳 true", true);
            EXPECT_TRUE(outAlbedo.x == 0.9f && outAlbedo.y == 0.1f && outAlbedo.z == 0.4f,
                        "手寫 v2 載入後 albedo 正確", true);
            EXPECT_TRUE(outMetallic == 0.7f, "手寫 v2 載入後 metallic 正確", true);
            EXPECT_TRUE(outRoughness == 0.2f, "手寫 v2 載入後 roughness 正確", true);

            // 載入 SavePrefab 輸出的 v2 檔
            Entity newRoot = system.LoadPrefab(Entity(), path);
            EXPECT_TRUE(newRoot.GetRaw() != 0, "SavePrefab v2 LoadPrefab 建立新根節點(raw != 0)", true);

            // 驗證根節點 PBR
            EXPECT_TRUE(system.GetPBR(newRoot.GetIndex(), outAlbedo, outMetallic, outRoughness),
                        "SavePrefab v2 載入後 GetPBR 回傳 true", true);
            EXPECT_TRUE(outAlbedo.x == 0.9f && outAlbedo.y == 0.1f && outAlbedo.z == 0.4f,
                        "SavePrefab v2 載入後 albedo 正確", true);
            EXPECT_TRUE(outMetallic == 0.7f, "SavePrefab v2 載入後 metallic 正確", true);
            EXPECT_TRUE(outRoughness == 0.2f, "SavePrefab v2 載入後 roughness 正確", true);

            // 驗證子節點 PBR
            DynamicArray<Entity> children = system.GetChildren(newRoot);
            EXPECT_TRUE(children.GetNElements() == 1, "根節點有 1 個子節點", true);
            EXPECT_TRUE(children[0].GetRaw() != 0, "子節點 raw != 0", true);
            EXPECT_TRUE(system.GetPBR(children[0].GetIndex(), outAlbedo, outMetallic, outRoughness),
                        "子節點 GetPBR 回傳 true", true);
            EXPECT_TRUE(outAlbedo.x == 0.3f && outAlbedo.y == 0.6f && outAlbedo.z == 0.9f,
                        "子節點 albedo 正確", true);
            EXPECT_TRUE(outMetallic == 0.1f, "子節點 metallic 正確", true);
            EXPECT_TRUE(outRoughness == 0.9f, "子節點 roughness 正確", true);

            std::remove(manualPath);
            std::remove(path);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }

  private:
};

} // namespace materialpbr_test

#endif // MATERIAL_PBR_TEST_HPP
