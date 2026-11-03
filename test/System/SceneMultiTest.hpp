#ifndef SCENE_MULTI_TEST_HPP
#define SCENE_MULTI_TEST_HPP

#include "Data/String.hpp"
#include "Display/GUI/EditorSession.hpp"
#include "Scene/SceneSystem.hpp"
#include "Test.hpp"

#include <cstdio>
#include <fstream>

namespace scenemulti_test
{

class SceneMultiTest : public Test
{
  public:
    SceneMultiTest() : Test("SceneMulti")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:PushScene/PopScene 場景堆疊基本操作 ----
        {
            SceneSystem system;
            Entity rootA = system.CreateNode();
            scenesystem_detail::NodeName nameA;
            scenesystem_detail::SetNodeName(nameA, "SceneA_Root");
            system.SetNodeName(rootA.GetIndex(), String(u"SceneA_Root"));

            EXPECT_TRUE(system.GetActiveScene() == "Default",
                        "初始場景名稱為 Default", true);
            EXPECT_TRUE(system.GetSceneStackDepth() == 0,
                        "初始場景堆疊深度為 0", true);

            // 壓入場景 B（目前場景 A 被序列化保存）
            EXPECT_TRUE(system.PushScene("SceneB"),
                        "PushScene(SceneB) 回傳 true", true);
            EXPECT_TRUE(system.GetActiveScene() == "SceneB",
                        "PushScene 後活躍場景為 SceneB", true);
            EXPECT_TRUE(system.GetSceneStackDepth() == 1,
                        "PushScene 後堆疊深度為 1", true);

            // 場景 B 是空的（剛剛清空了世界）
            DynamicArray<Entity> rootsB = system.GetRoots();
            EXPECT_TRUE(rootsB.GetNElements() == 0,
                        "場景 B 應為空場景", true);

            // 在場景 B 中建立節點
            Entity rootB = system.CreateNode();
            scenesystem_detail::NodeName nameB;
            scenesystem_detail::SetNodeName(nameB, "SceneB_Root");
            system.SetNodeName(rootB.GetIndex(), String(u"SceneB_Root"));

            // 再壓入場景 C
            EXPECT_TRUE(system.PushScene("SceneC"),
                        "PushScene(SceneC) 回傳 true", true);
            EXPECT_TRUE(system.GetSceneStackDepth() == 2,
                        "兩次 PushScene 後堆疊深度為 2", true);

            // 彈出場景 C，應恢復場景 B（包含 rootB 的序列化狀態）
            EXPECT_TRUE(system.PopScene(),
                        "PopScene 回傳 true", true);
            EXPECT_TRUE(system.GetActiveScene() == "SceneB",
                        "PopScene 後活躍場景為 SceneB", true);
            EXPECT_TRUE(system.GetSceneStackDepth() == 1,
                        "PopScene 後堆疊深度為 1", true);

            // 驗證場景 B 的節點仍然存在（透過 GetRoots 重新取得 handle，
            // 因為 PopScene 會重建世界，舊 handle 已失效）
            DynamicArray<Entity> rootsBRestored = system.GetRoots();
            EXPECT_TRUE(rootsBRestored.GetNElements() == 1,
                        "PopScene 後場景 B 有 1 個根節點", true);
            String rootBRestoredName;
            EXPECT_TRUE(system.GetNodeName(rootsBRestored[0].GetIndex(), rootBRestoredName),
                        "PopScene 後可查詢場景 B 根節點名稱", true);

            // 再彈出場景 B，應恢復場景 A（包含 rootA 的序列化狀態）
            EXPECT_TRUE(system.PopScene(),
                        "第二次 PopScene 回傳 true", true);
            EXPECT_TRUE(system.GetActiveScene() == "Default",
                        "第二次 PopScene 後活躍場景為 Default", true);
            EXPECT_TRUE(system.GetSceneStackDepth() == 0,
                        "第二次 PopScene 後堆疊深度為 0", true);

            // 驗證場景 A 的節點仍然存在（透過 GetRoots 重新取得 handle）
            DynamicArray<Entity> rootsARestored = system.GetRoots();
            EXPECT_TRUE(rootsARestored.GetNElements() == 1,
                        "第二次 PopScene 後場景 A 有 1 個根節點", true);
            String rootARestoredName;
            EXPECT_TRUE(system.GetNodeName(rootsARestored[0].GetIndex(), rootARestoredName),
                        "第二次 PopScene 後可查詢場景 A 根節點名稱", true);

            // 舊的 rootA / rootB handle 在 PopScene 後應已失效（世界被重建）
            EXPECT_TRUE(!system.Alive(rootA),
                        "場景 A 卸載後舊 rootA handle 應失效", true);
            EXPECT_TRUE(!system.Alive(rootB),
                        "場景 B 卸載後舊 rootB handle 應失效", true);
        }

        // ---- AC2:PopScene 空堆疊回 false ----
        {
            SceneSystem system;
            EXPECT_TRUE(!system.PopScene(),
                        "空堆疊 PopScene 回傳 false", true);
        }

        // ---- AC3:PushScene nullptr 回 false ----
        {
            SceneSystem system;
            EXPECT_TRUE(!system.PushScene(nullptr),
                        "PushScene(nullptr) 回傳 false", true);
        }

        // ---- AC4:EditorSession.LoadSceneFile/UnloadCurrentScene 集成 ----
        {
            SceneSystem system;
            EditorSession session(&system, nullptr, nullptr);

            Entity root = system.CreateNode();
            system.SetMaterial(root.GetIndex(), 7);
            session.Select(root.GetIndex());

            const char *path = TestTempFilePath("chia_scenemulti_test.scn");
            EXPECT_TRUE(system.SaveScene(path),
                        "SaveScene 回傳 true", true);

            // 清空場景
            system.DestroyNode(root);
            EXPECT_TRUE(!system.Alive(root),
                        "DestroyNode 後 root 失效", true);

            // 從檔案載入場景
            EXPECT_TRUE(session.LoadSceneFile(String(path)),
                        "EditorSession::LoadSceneFile 回傳 true", true);

            // 驗證場景已恢復（場景 v1 格式不保存 materialId，僅驗證節點存在）
            DynamicArray<Entity> roots = system.GetRoots();
            EXPECT_TRUE(roots.GetNElements() == 1,
                        "載入後場景有 1 個根節點", true);

            std::remove(path);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }

  private:
};

} // namespace scenemulti_test

#endif // SCENE_MULTI_TEST_HPP
