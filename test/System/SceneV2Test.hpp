#ifndef SCENE_V2_TEST_HPP
#define SCENE_V2_TEST_HPP

#include "Data/String.hpp"
#include "Display/GUI/EditorSession.hpp"
#include "Scene/SceneSystem.hpp"
#include "Test.hpp"

#include <cstdio>
#include <fstream>

namespace scenev2_test
{

class SceneV2Test : public Test
{
  public:
    SceneV2Test() : Test("SceneV2")
    {
    }

    static void WriteTestPrefab(const char *path)
    {
        std::ofstream file(path);
        file << "prefab v1\n";
        // root: localIndex 0, parent -1, materialId 0, name "PrefabRoot"
        file << "node 0 -1 0 PrefabRoot 0 0 0 0 0 0 1 1 1\n";
        // child: localIndex 1, parent 0, materialId 5, name "PrefabChild"
        file << "node 1 0 5 PrefabChild 10 20 30 0 0 0 1 1 1\n";
        file << "end\n";
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:場景 v2 儲存/載入往返(無 prefab_ref) ----
        {
            SceneSystem system;
            Entity root = system.CreateNode();
            scenesystem_detail::NodeName name;
            scenesystem_detail::SetNodeName(name, "SceneRoot");
            system.SetNodeName(root.GetIndex(), String(u"SceneRoot"));

            TransformComponent tc;
            tc.position = Point3D(1, 2, 3);
            *system.GetLocalTransform(root) = tc;

            const char *path = TestTempFilePath("chia_scene_v2_test.scn");
            EXPECT_TRUE(system.SaveScene(path), "SaveScene v2 回傳 true", true);

            // 清空後重新載入
            system.DestroyNode(root);
            EXPECT_TRUE(!system.Alive(root), "DestroyNode 後 root 失效", true);

            EXPECT_TRUE(system.LoadScene(path), "LoadScene v2 回傳 true", true);

            DynamicArray<Entity> roots = system.GetRoots();
            EXPECT_TRUE(roots.GetNElements() == 1, "v2 載入後有 1 個根節點", true);
            String rootName;
            EXPECT_TRUE(system.GetNodeName(roots[0].GetIndex(), rootName),
                        "v2 載入後可查詢根節點名稱", true);

            TransformComponent loadedTc;
            EXPECT_TRUE(system.GetTransform(roots[0].GetIndex(), loadedTc),
                        "v2 載入後可查詢 transform", true);
            EXPECT_TRUE(loadedTc.position.x == 1.f && loadedTc.position.y == 2.f &&
                          loadedTc.position.z == 3.f,
                        "v2 載入後位置正確", true);

            std::remove(path);
        }

        // ---- AC2:prefab_ref 跨場景引用 ----
        {
            SceneSystem system;

            // 建立測試 prefab 檔
            const char *prefabPath = TestTempFilePath("chia_scene_v2_test.prefab");
            WriteTestPrefab(prefabPath);

            // 透過 LoadPrefab 實例化 prefab 到場景中
            Entity prefabRoot = system.LoadPrefab(Entity(), prefabPath);
            EXPECT_TRUE(system.Alive(prefabRoot),
                        "LoadPrefab 實例化 prefab 根節點", true);

            // 儲存場景 v2，應包含 prefab_ref 行
            const char *scenePath = TestTempFilePath("chia_scene_v2_with_ref.scn");
            EXPECT_TRUE(system.SaveScene(scenePath), "SaveScene v2 回傳 true", true);

            // 清空場景
            DynamicArray<Entity> allRoots = system.GetRoots();
            for (size_t i = 0; i < allRoots.GetNElements(); ++i)
                system.DestroyNode(allRoots[i]);

            // 重新載入場景 v2，prefab 子樹應被實例化
            EXPECT_TRUE(system.LoadScene(scenePath), "LoadScene v2 with prefab_ref 回傳 true", true);

            // 驗證 prefab 子樹存在
            DynamicArray<Entity> rootsAfterReload = system.GetRoots();
            EXPECT_TRUE(rootsAfterReload.GetNElements() >= 1,
                        "v2 with prefab_ref 載入後至少有 1 個根節點", true);

            bool foundPrefabRoot = false;
            for (size_t i = 0; i < rootsAfterReload.GetNElements(); ++i)
            {
                String rn;
                if (system.GetNodeName(rootsAfterReload[i].GetIndex(), rn) &&
                    rn == String(u"PrefabRoot"))
                {
                    foundPrefabRoot = true;
                    break;
                }
            }
            EXPECT_TRUE(foundPrefabRoot, "找到 PrefabRoot 節點", true);

            // 驗證 prefab 子節點存在
            bool foundPrefabChild = false;
            for (size_t i = 0; i < rootsAfterReload.GetNElements(); ++i)
            {
                DynamicArray<Entity> children = system.GetChildren(rootsAfterReload[i]);
                for (size_t j = 0; j < children.GetNElements(); ++j)
                {
                    String cn;
                    if (system.GetNodeName(children[j].GetIndex(), cn) &&
                        cn == String(u"PrefabChild"))
                    {
                        foundPrefabChild = true;
                        break;
                    }
                }
                if (foundPrefabChild)
                    break;
            }
            EXPECT_TRUE(foundPrefabChild, "找到 PrefabChild 節點", true);

            std::remove(prefabPath);
            std::remove(scenePath);
        }

        // ---- AC3:v1 向下相容 ----
        {
            SceneSystem system;
            Entity root = system.CreateNode();

            const char *v1Path = TestTempFilePath("chia_scene_v1_test.scn");
            // 手寫 v1 格式檔案
            std::ofstream v1File(v1Path);
            v1File << "scene v1\n";
            v1File << "node 0 -1 V1Root 5 5 5 0 0 0 1 1 1\n";
            v1File << "end\n";
            v1File.close();

            system.DestroyNode(root);
            EXPECT_TRUE(system.LoadScene(v1Path), "LoadScene v1 回傳 true", true);

            DynamicArray<Entity> v1Roots = system.GetRoots();
            EXPECT_TRUE(v1Roots.GetNElements() == 1,
                        "v1 載入後有 1 個根節點", true);
            String v1RootName;
            EXPECT_TRUE(system.GetNodeName(v1Roots[0].GetIndex(), v1RootName),
                        "v1 載入後可查詢名稱", true);

            std::remove(v1Path);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }

  private:
};

} // namespace scenev2_test

#endif // SCENE_V2_TEST_HPP
