#ifndef SCENE_RUNTIME_TEST_HPP
#define SCENE_RUNTIME_TEST_HPP

#include "Scene/SceneSystem.hpp"
#include "Test.hpp"

#include <cstdio>
#include <fstream>

namespace sceneruntime_test
{

// SceneRuntimeTest:A3.3 場景運行時契約。
// 1. SetSceneActive/IsSceneActive 往返
// 2. DontDestroyOnLoad 標記與查詢
// 3. 載入新場景後 DontDestroyOnLoad 物件仍存活
// 4. 非 DontDestroyOnLoad 物件在場景卸載後銷毀
class SceneRuntimeTest : public Test
{
  public:
    SceneRuntimeTest() : Test("SceneRuntime")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:SetSceneActive/IsSceneActive 往返 ----
        {
            SceneSystem system;
            EXPECT_TRUE(system.IsSceneActive(), "預設場景 active=true", true);

            system.SetSceneActive(false);
            EXPECT_TRUE(!system.IsSceneActive(), "SetSceneActive(false) → false", true);

            system.SetSceneActive(true);
            EXPECT_TRUE(system.IsSceneActive(), "SetSceneActive(true) → true", true);
        }

        // ---- AC2:DontDestroyOnLoad 標記與查詢 ----
        {
            SceneSystem system;
            Entity e = system.CreateNode();

            EXPECT_TRUE(!system.IsDontDestroyOnLoad(e), "預設 DontDestroyOnLoad=false", true);

            system.SetDontDestroyOnLoad(e, true);
            EXPECT_TRUE(system.IsDontDestroyOnLoad(e), "SetDontDestroyOnLoad(true) → true", true);

            system.SetDontDestroyOnLoad(e, false);
            EXPECT_TRUE(!system.IsDontDestroyOnLoad(e), "SetDontDestroyOnLoad(false) → false", true);
        }

        // ---- AC3:載入新場景後 DontDestroyOnLoad 物件仍存活 ----
        {
            SceneSystem system;

            Entity ddo = system.CreateNode();
            system.SetDontDestroyOnLoad(ddo, true);

            Entity normal = system.CreateNode();
            system.SetDontDestroyOnLoad(normal, false);

            // 建立一個最小有效場景檔,然後 LoadScene
            const char *goodPath = TestTempFilePath("chia_scene_good.scene");
            {
                std::ofstream out(goodPath);
                out << "scene v1\n";
                out << "node 0 -1 NewRoot 0 0 0 0 0 0 1 1 1\n";
                out << "end\n";
            }
            EXPECT_TRUE(system.LoadScene(goodPath), "LoadScene 有效檔案 → true", true);

            EXPECT_TRUE(system.Alive(ddo), "DontDestroyOnLoad 物件在 LoadScene 後仍存活", true);
            // normal 被卸載,新場景建立了新節點
            EXPECT_TRUE(!system.Alive(normal),
                        "非 DontDestroyOnLoad 物件在 LoadScene 後被銷毀", true);

            std::remove(goodPath);
        }

        // ---- AC4:UnloadScene 保留 DontDestroyOnLoad 物件 ----
        {
            SceneSystem system;

            Entity ddo = system.CreateNode();
            system.SetDontDestroyOnLoad(ddo, true);

            Entity normal = system.CreateNode();
            system.SetDontDestroyOnLoad(normal, false);

            system.UnloadScene();

            EXPECT_TRUE(system.Alive(ddo), "DontDestroyOnLoad 物件在 UnloadScene 後仍存活", true);
            EXPECT_TRUE(!system.Alive(normal),
                        "非 DontDestroyOnLoad 物件在 UnloadScene 後被銷毀", true);
        }

        // ---- AC5:DontDestroyOnLoad 子物件也被保留(父為 DontDestroyOnLoad) ----
        {
            SceneSystem system;

            Entity parent = system.CreateNode();
            system.SetDontDestroyOnLoad(parent, true);

            Entity child = system.CreateNode(parent);
            // 子沒有設定 DontDestroyOnLoad，但因為父是 DontDestroyOnLoad，
            // UnloadScene 只檢查 roots，parent 是 root 且為 DDOL → 不會銷毀
            // → 整棵子樹都保留

            system.UnloadScene();

            EXPECT_TRUE(system.Alive(parent), "DDOL 父節點存活", true);
            EXPECT_TRUE(system.Alive(child), "DDOL 父節點的子節點也存活", true);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }

  private:
};

} // namespace sceneruntime_test

#endif // SCENE_RUNTIME_TEST_HPP
