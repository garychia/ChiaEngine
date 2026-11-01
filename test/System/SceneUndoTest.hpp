#ifndef SCENE_UNDO_TEST_HPP
#define SCENE_UNDO_TEST_HPP

#include "Data/String.hpp"
#include "Display/GUI/EditorSession.hpp"
#include "Display/GUI/SceneEditCommand.hpp"
#include "Scene/SceneSystem.hpp"
#include "Test.hpp"

#include <cstdint>

namespace sceneundo_test
{

class SceneUndoTest : public Test
{
  public:
    SceneUndoTest() : Test("SceneUndo")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:CreateNodeCommand/Apply+Revert ----
        {
            SceneSystem system;
            CreateNodeCommand cmd(&system, Entity());
            cmd.Apply();
            // 新節點應可查詢(索引可能為 0,因 EntityRegistry 從 slot 0 開始分配)
            uint64_t matId = 0;
            EXPECT_TRUE(system.GetMaterial(cmd.GetCreatedEntityIndex(), matId),
                        "CreateNode 產生可查詢的節點", true);
            EXPECT_TRUE(matId == 0, "新節點 materialId 預設為 0", true);

            cmd.Revert();
            // Revert 後節點應被銷毀:查詢 materialId 回 false
            EXPECT_TRUE(!system.GetMaterial(cmd.GetCreatedEntityIndex(), matId),
                        "Revert 後節點已銷毀,GetMaterial 回 false", true);
        }

        // ---- AC2:DestroyNodeCommand/Apply+Revert ----
        {
            SceneSystem system;
            Entity root = system.CreateNode();
            Entity child = system.CreateNode(root);

            DestroyNodeCommand cmd(&system, root);
            cmd.Apply();
            // Root 與 child 都應被銷毀
            uint64_t matId = 0;
            EXPECT_TRUE(!system.GetMaterial(root.GetIndex(), matId),
                        "DestroyNode 連同子樹銷毀:root 失效", true);
            EXPECT_TRUE(!system.GetMaterial(child.GetIndex(), matId),
                        "DestroyNode 連同子樹銷毀:child 失效", true);

            cmd.Revert();
            // Revert 後 root 重建(新 index),materialId 回到 0
            uint32_t newIdx = cmd.GetCreatedEntityIndex();
            // 新節點應可查詢(索引可能為 0)
            EXPECT_TRUE(system.GetMaterial(newIdx, matId), "重建節點可查詢 material", true);
            EXPECT_TRUE(matId == 0, "重建節點 materialId 為 0", true);
        }

        // ---- AC3:SetParentCommand/Apply+Revert ----
        {
            SceneSystem system;
            Entity root = system.CreateNode();
            Entity child = system.CreateNode();
            // child 初始是 root 的兄弟(parent = null)

            SetParentCommand cmd(&system, child, root);
            cmd.Apply();
            EXPECT_TRUE(system.GetParent(child) == root,
                        "Apply 後 child.parent == root", true);

            cmd.Revert();
            EXPECT_TRUE(system.GetParent(child).GetRaw() == 0,
                        "Revert 後 child.parent 回到 null(root)", true);

            cmd.Apply(); // Redo
            EXPECT_TRUE(system.GetParent(child) == root,
                        "Re-Apply 後 child.parent == root", true);
        }

        // ---- AC4:SetMaterialCommand/Apply+Revert ----
        {
            SceneSystem system;
            Entity root = system.CreateNode();

            SetMaterialCommand cmd(&system, root.GetIndex(), 42);
            cmd.Apply();
            uint64_t matId = 0;
            EXPECT_TRUE(system.GetMaterial(root.GetIndex(), matId), "GetMaterial 回 true", true);
            EXPECT_TRUE(matId == 42, "Apply 後 materialId == 42", true);

            cmd.Revert();
            matId = 999;
            EXPECT_TRUE(system.GetMaterial(root.GetIndex(), matId), "GetMaterial 回 true", true);
            EXPECT_TRUE(matId == 0, "Revert 後 materialId 回到 0", true);

            cmd.Apply(); // Redo
            matId = 0;
            EXPECT_TRUE(system.GetMaterial(root.GetIndex(), matId), "GetMaterial 回 true", true);
            EXPECT_TRUE(matId == 42, "Re-Apply 後 materialId == 42", true);
        }

        // ---- AC5:EditorSession.CreateNode/DestroyNode/SetParent/SetSelectedEntityMaterial 經 UndoStack ----
        {
            SceneSystem system;
            EditorSession session(&system, nullptr, nullptr);

            // CreateNode via UndoStack
            uint32_t idx = session.CreateNode(Entity());
            // 新節點應可查詢(索引可能為 0)
            uint64_t matId = 0;
            EXPECT_TRUE(system.GetMaterial(idx, matId), "EditorSession::CreateNode 建立可查詢節點", true);
            EXPECT_TRUE(session.GetUndoStack().GetUndoCount() == 1,
                        "UndoStack 有一筆可 undo", true);

            // Undo: 節點被銷毀
            session.GetUndoStack().Undo();
            EXPECT_TRUE(!system.GetMaterial(idx, matId),
                        "Undo 後節點銷毀,GetMaterial 回 false", true);
            EXPECT_TRUE(session.GetUndoStack().GetRedoCount() == 1,
                        "UndoStack 有一筆可 redo", true);

            // Redo: 節點重建(新 index)
            session.GetUndoStack().Redo();
            EXPECT_TRUE(session.GetUndoStack().GetUndoCount() == 1,
                        "Redo 後 undo count 回到 1", true);
            EXPECT_TRUE(session.GetUndoStack().GetRedoCount() == 0,
                        "Redo 後 redo count 為 0", true);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }
};

} // namespace sceneundo_test

#endif // SCENE_UNDO_TEST_HPP
