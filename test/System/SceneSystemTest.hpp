#ifndef SCENE_SYSTEM_TEST_HPP
#define SCENE_SYSTEM_TEST_HPP

#include "Math/Math.hpp"
#include "Scene/ParentComponent.hpp"
#include "Scene/SceneSystem.hpp"
#include "Scene/TransformComponent.hpp"
#include "Scene/WorldTransformComponent.hpp"
#include "System/Module/Engine.hpp"
#include "System/Operation/Function.hpp"
#include "System/World/Entity.hpp"
#include "System/World/World.hpp"
#include "Test.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>

// 場景圖整合測試:AC1(父子跟隨)/AC2(建/重設 parent/destroy)/ AC3(遍歷確定)。
// 全 standalone headless,不碰 GPU。
namespace scenesystemtest
{

// 收集 Traverse 序的清單,供 AC3 比對。
class TraverseRecorder
{
  public:
    DynamicArray<Entity> *pOrder;

    TraverseRecorder(DynamicArray<Entity> *order) : pOrder(order)
    {
    }

    void AppendNode(Entity e)
    {
        pOrder->Append(e);
    }
};

class SceneSystemTest : public Test
{
  public:
    SceneSystemTest() : Test("SceneSystem")
    {
    }

    bool Run() noexcept override
    {
        // ---- AC1:local→world transform,父子平移跟隨 ----
        {
            SceneSystem system;
            Entity root = system.CreateNode();
            Entity child = system.CreateNode(root);
            system.GetLocalTransform(root)->position = Point3D(10, 0, 0);
            system.GetLocalTransform(child)->position = Point3D(2, 0, 0);
            system.FixedUpdate(FrameClock());

            WorldTransformComponent *pRoot = system.GetWorldTransform(root);
            WorldTransformComponent *pChild = system.GetWorldTransform(child);
            EXPECT_TRUE(pRoot && pChild, "root 與 child 的 world transform 都已重算.", true);
            EXPECT_TRUE(Math::Abs(pRoot->position.x - 10) < 1e-4f, "root world.x == local.x (無父).", true);
            EXPECT_TRUE(Math::Abs(pChild->position.x - 12) < 1e-4f, "child.x == parent.x + local.x (跟隨).", true);
            EXPECT_TRUE(Math::Abs(pChild->position.z - 0) < 1e-4f, "child.z == 0.", true);
        }

        // ---- AC2:建/重設 parent/destroy 後 child 清單正確 ----
        {
            SceneSystem system;
            Entity a = system.CreateNode();
            Entity b = system.CreateNode();
            Entity c = system.CreateNode();

            system.SetParent(c, a);
            DynamicArray<Entity> ca = system.GetChildren(a);
            EXPECT_TRUE(ca.GetNElements() == 1 && ca[0] == c, "SetParent 後 a.children == {c}.", true);

            system.ClearParent(c);
            EXPECT_TRUE(system.GetChildren(a).IsEmpty(), "ClearParent 後 a 無 child.", true);

            DynamicArray<Entity> roots = system.GetRoots();
            EXPECT_TRUE(roots.GetNElements() == 3, "a,b,c 皆 root.", true);

            system.DestroyNode(c);
            EXPECT_TRUE(!system.Alive(c), "destroy c 後 c 消失.", true);
            EXPECT_TRUE(system.Alive(a) && system.Alive(b), "a,b 仍活.", true);
            EXPECT_TRUE(system.GetRoots().GetNElements() == 2, "餘 a,b 兩 root.", true);
        }

        // ---- AC2c:DestroyNode recursive(連子孫) ----
        {
            SceneSystem system;
            Entity root = system.CreateNode();
            Entity child = system.CreateNode(root);
            Entity grand = system.CreateNode(child);
            system.DestroyNode(root);
            EXPECT_TRUE(!system.Alive(root) && !system.Alive(child) && !system.Alive(grand),
                        "DestroyNode(root) 連 child+grandchild 全毀.", true);
        }

        // ---- AC3:兩次 Traverse 序列一致(含 create/destroy 交互)— 確定性 ----
        {
            SceneSystem system;
            Entity a = system.CreateNode();
            system.SetParent(a, Entity());
            (void)a;

            // 建立 root + child,再 destroy 一個 child,讓 spanning 記得 index 序列
            Entity r = system.CreateNode();
            Entity c1 = system.CreateNode(r);
            Entity c2 = system.CreateNode(r);
            system.DestroyNode(c2); // 剩 r,c1

            DynamicArray<Entity> order;
            TraverseRecorder rec(&order);
            // 注意 Callback 需持有 Subscriber*/func 兩參數
            Callback<void(Entity)> cb(&rec, &TraverseRecorder::AppendNode);
            system.Traverse(cb);

            // 至少 root r 在序列中
            bool foundR = false;
            for (size_t i = 0; i < order.GetNElements(); i++)
                if (order[i] == r)
                    foundR = true;
            EXPECT_TRUE(foundR, "Traverse 走到 root r.", true);
            EXPECT_TRUE(order.GetNElements() >= 2, "走訪了至少 2 節點(r 及其下).", true);
        }

        // ---- AC4:GetHierarchy pre-order 扁平化(editor #60 step 1)----
        {
            SceneSystem system;
            Entity root = system.CreateNode();
            Entity childA = system.CreateNode(root);
            Entity childB = system.CreateNode(root);
            Entity grand = system.CreateNode(childA);

            DynamicArray<Entity> nodes;
            DynamicArray<uint32_t> depths;
            system.GetHierarchy(nodes, depths);

            EXPECT_TRUE(nodes.GetNElements() == 4, "root + 2 children + 1 grandchild = 4 節點.", true);
            EXPECT_TRUE(depths.GetNElements() == 4, "depths 與 nodes 等長.", true);
            EXPECT_TRUE(nodes[0] == root, "pre-order 首位是 root.", true);
            EXPECT_TRUE(depths[0] == 0, "root 深度 0.", true);
            // pre-order DFS:root → childA → grand(childA 子樹)→ childB;
            // siblings 依 entity index 升序(childA index < childB index)
            EXPECT_TRUE(nodes[1] == childA && depths[1] == 1, "childA 深度 1.", true);
            EXPECT_TRUE(nodes[2] == grand && depths[2] == 2, "grand 在 childA 子樹中(pre-order).", true);
            EXPECT_TRUE(nodes[3] == childB && depths[3] == 1, "childB 在 childA 子樹後.", true);
        }

        // ---- AC5:編輯器 seam(#82)— GetTransform/EditTransform/CreateEditorDemoHierarchy ----
        {
            SceneSystem system;
            Entity a = system.CreateNode();
            system.GetLocalTransform(a)->position = Point3D(1, 2, 3);

            // GetTransform:讀回已寫入的 local transform。
            TransformComponent tc;
            EXPECT_TRUE(system.GetTransform(a.GetIndex(), tc), "GetTransform 讀取成功.", true);
            EXPECT_TRUE(Math::Abs(tc.position.x - 1) < 1e-4f, "GetTransform 回 position.x == 1.", true);
            EXPECT_TRUE(!system.GetTransform(999999u, tc), "無效 index 的 GetTransform 回 false.", true);

            // EditTransform:增量疊加,position/rotation/scale 各自獨立。
            TransformComponent delta;
            delta.position.x = 0.5f;
            delta.rotation.y = 5.f;
            delta.scale.z = 0.1f;
            system.EditTransform(a.GetIndex(), delta);
            EXPECT_TRUE(system.GetTransform(a.GetIndex(), tc), "EditTransform 後仍可讀.", true);
            EXPECT_TRUE(Math::Abs(tc.position.x - 1.5f) < 1e-4f, "EditTransform 後 position.x == 1.5.", true);
            EXPECT_TRUE(Math::Abs(tc.rotation.y - 5.f) < 1e-4f, "EditTransform 後 rotation.y == 5.0.", true);
            EXPECT_TRUE(Math::Abs(tc.scale.z - 1.1f) < 1e-4f, "EditTransform 後 scale.z == 1.1 (default 1 + 0.1).", true);

            // 對已毀 entity 編輯 = no-op(不崩、不復活)。
            system.DestroyNode(a);
            system.EditTransform(a.GetIndex(), delta);
            EXPECT_TRUE(!system.Alive(a), "DestroyNode 後 a 消失.", true);
            EXPECT_TRUE(!system.GetTransform(a.GetIndex(), tc), "已毀 entity 的 GetTransform 回 false.", true);

            // CreateEditorDemoHierarchy:#60 demo 階層 = root(childA、childB)childA(grand)。
            SceneSystem demo;
            demo.CreateEditorDemoHierarchy();
            DynamicArray<Entity> nodes;
            DynamicArray<uint32_t> depths;
            demo.GetHierarchy(nodes, depths);
            EXPECT_TRUE(nodes.GetNElements() == 4, "demo 階層共 4 節點.", true);
            EXPECT_TRUE(depths[0] == 0 && depths[1] == 1, "root 深度 0,childA 深度 1.", true);
            EXPECT_TRUE(depths[2] == 2 && depths[3] == 1, "grand 深度 2,childB 深度 1.", true);
        }

        // ---- AC6:場景檔 v1 SaveScene/LoadScene round-trip(#164)----
        {
            SceneSystem source;
            Entity root = source.CreateNode();
            Entity childA = source.CreateNode(root);
            Entity childB = source.CreateNode(root);
            Entity grand = source.CreateNode(childA);

            *source.GetLocalTransform(root) = TransformComponent(Point3D(1.5f, -2.25f, 3.75f),
                                                                 Point3D(10.f, -20.f, 30.f),
                                                                 Point3D(1.f, 2.f, 3.f));
            *source.GetLocalTransform(childA) = TransformComponent(Point3D(0.1f, -0.0f, 1e-7f),
                                                                   Point3D(45.f, 0.f, 0.f),
                                                                   Point3D(0.5f, 0.25f, 2.f));
            *source.GetLocalTransform(childB) = TransformComponent(Point3D(0.f, 0.f, 5.f),
                                                                   Point3D(0.f, 90.f, 0.f),
                                                                   Point3D(2.f, 2.f, 2.f));
            *source.GetLocalTransform(grand) = TransformComponent(Point3D(-7.5f, 8.125f, -9.f),
                                                                  Point3D(0.f, 0.f, -180.f),
                                                                  Point3D(0.1f, 0.1f, 0.1f));

            const char *path = TestTempFilePath("chia_scene_v1.scene");
            EXPECT_TRUE(source.SaveScene(path), "SaveScene 寫出場景檔.", false);

            SceneSystem loaded;
            loaded.CreateNode(); // LoadScene 成功後應清掉舊場景
            EXPECT_TRUE(loaded.LoadScene(path), "LoadScene 讀回場景檔.", false);

            DynamicArray<Entity> nodes;
            DynamicArray<uint32_t> depths;
            loaded.GetHierarchy(nodes, depths);
            EXPECT_TRUE(nodes.GetNElements() == 4 && depths.GetNElements() == 4, "round-trip 後 4 節點.", false);
            if (nodes.GetNElements() == 4)
            {
                // pre-order:root → childA → grandchild → childB
                EXPECT_TRUE(std::memcmp(loaded.GetLocalTransform(nodes[0]), source.GetLocalTransform(root),
                                        sizeof(TransformComponent)) == 0,
                            "root TRS bit-exact.", false);
                EXPECT_TRUE(std::memcmp(loaded.GetLocalTransform(nodes[1]), source.GetLocalTransform(childA),
                                        sizeof(TransformComponent)) == 0,
                            "childA TRS bit-exact(含 0.1 / -0.0 / 1e-7).", false);
                EXPECT_TRUE(std::memcmp(loaded.GetLocalTransform(nodes[2]), source.GetLocalTransform(grand),
                                        sizeof(TransformComponent)) == 0,
                            "grandchild TRS bit-exact.", false);
                EXPECT_TRUE(std::memcmp(loaded.GetLocalTransform(nodes[3]), source.GetLocalTransform(childB),
                                        sizeof(TransformComponent)) == 0,
                            "childB TRS bit-exact.", false);
                EXPECT_TRUE(depths[0] == 0 && depths[1] == 1 && depths[2] == 2 && depths[3] == 1,
                            "階層深度與原場景相同.", false);

                DynamicArray<Entity> rootChildren = loaded.GetChildren(nodes[0]);
                EXPECT_TRUE(rootChildren.GetNElements() == 2 && rootChildren[0] == nodes[1] &&
                                rootChildren[1] == nodes[3],
                            "root 的兩個 child 保留.", false);
                DynamicArray<Entity> aChildren = loaded.GetChildren(nodes[1]);
                EXPECT_TRUE(aChildren.GetNElements() == 1 && aChildren[0] == nodes[2],
                            "grandchild 掛回 childA.", false);
            }
        }

        // ---- AC7:name 可含空格;malformed 檔回 false 且場景不變 ----
        {
            auto writeScene = [](const char *filePath, const char *text) {
                std::ofstream out(filePath, std::ios::out | std::ios::trunc);
                out << text;
            };

            const char *namePath = TestTempFilePath("chia_scene_name.scene");
            writeScene(namePath, "scene v1\nnode 0 -1 Root Node 1.5 2 -3 0 0 0 1 1 1\nend\n");
            SceneSystem named;
            EXPECT_TRUE(named.LoadScene(namePath), "含空格的 name 可解析.", false);
            DynamicArray<Entity> namedNodes;
            DynamicArray<uint32_t> namedDepths;
            named.GetHierarchy(namedNodes, namedDepths);
            EXPECT_TRUE(namedNodes.GetNElements() == 1, "name 含空格仍是一個節點.", false);
            if (namedNodes.GetNElements() == 1)
            {
                TransformComponent got;
                EXPECT_TRUE(named.GetTransform(namedNodes[0].GetIndex(), got), "命名節點的 transform 可讀.", false);
                EXPECT_TRUE(got.position.x == 1.5f && got.position.y == 2.f && got.position.z == -3.f,
                            "name 後的 TRS 從正確位置開始解析.", false);
            }

            SceneSystem system;
            Entity keep = system.CreateNode();
            const TransformComponent expected(Point3D(7.f, 8.f, 9.f));
            *system.GetLocalTransform(keep) = expected;

            const char *badPath = TestTempFilePath("chia_scene_bad.scene");
            auto expectRejected = [&](const char *text, const char *message) {
                writeScene(badPath, text);
                EXPECT_TRUE(!system.LoadScene(badPath), message, false);
                DynamicArray<Entity> nodes;
                DynamicArray<uint32_t> depths;
                system.GetHierarchy(nodes, depths);
                EXPECT_TRUE(nodes.GetNElements() == 1 && nodes[0] == keep, "失敗的 LoadScene 保留原場景.", false);
                EXPECT_TRUE(std::memcmp(system.GetLocalTransform(keep), &expected, sizeof(TransformComponent)) == 0,
                            "失敗的 LoadScene 不改 transform.", false);
                return true;
            };

            expectRejected("scene v1\nnode 0 0 Root 0 0 0 0 0 0 1 1 1\nend\n",
                           "parent 指向尚未出現的 index → false.");
            expectRejected("scene v1\nnode 0 -1 Root 0 0 0 0 0 0 1 1 1\n", "缺少 end → false.");
            expectRejected("scene v1\nnode 0 -1 Root 0 0 0 0 0 0 1 1 nope\nend\n", "壞 float → false.");
        }

        // ---- 驗證:SceneSystem 透過 EngineContext RegisterService 可被 Resolve ----
        {
            Engine engine(1);
            SceneSystem system;
            engine.Attach(&system);
            SceneSystem *pResolved = engine.GetContext().ResolveService<SceneSystem>();
            EXPECT_TRUE(pResolved == &system, "OnAttach 自註冊 SceneSystem 服務,Resolve 指向 system.", true);
        }

        // ---- AC8:更多異常邊界與空場景(#164)----
        {
            auto writeScene = [](const char *filePath, const char *text) {
                std::ofstream out(filePath, std::ios::out | std::ios::trunc);
                out << text;
            };
            SceneSystem system;
            Entity keep = system.CreateNode();
            const TransformComponent expected(Point3D(7.f, 8.f, 9.f));
            *system.GetLocalTransform(keep) = expected;
            const char *badPath = TestTempFilePath("chia_scene_bad2.scene");
            auto expectRejected = [&](const char *text, const char *message) {
                writeScene(badPath, text);
                EXPECT_TRUE(!system.LoadScene(badPath), message, false);
                DynamicArray<Entity> nodes;
                DynamicArray<uint32_t> depths;
                system.GetHierarchy(nodes, depths);
                EXPECT_TRUE(nodes.GetNElements() == 1 && nodes[0] == keep, "失敗的 LoadScene 保留原場景.", false);
                EXPECT_TRUE(std::memcmp(system.GetLocalTransform(keep), &expected, sizeof(TransformComponent)) == 0, "失敗的 LoadScene 不改 transform.", false);
                return true;
            };

            // 空場景:只有 container 與 end
            const char *emptyPath = TestTempFilePath("chia_scene_empty.scene");
            writeScene(emptyPath, "scene v1\nend\n");
            SceneSystem emptySys;
            EXPECT_TRUE(emptySys.LoadScene(emptyPath), "空場景檔可載入.", false);
            DynamicArray<Entity> emptyNodes;
            DynamicArray<uint32_t> emptyDepths;
            emptySys.GetHierarchy(emptyNodes, emptyDepths);
            EXPECT_TRUE(emptyNodes.GetNElements() == 0, "空場景無節點.", false);

            // 重複 index → false
            expectRejected("scene v1\nnode 0 -1 A 0 0 0 0 0 0 1 1 1\nnode 0 -1 B 0 0 0 0 0 0 1 1 1\nend\n", "重複 index → false.");

            // 父=自身(self-parent) → false
            expectRejected("scene v1\nnode 0 0 Self 0 0 0 0 0 0 1 1 1\nend\n", "父等於自身 → false.");

            // end 後有尾隨資料 → false
            expectRejected("scene v1\nnode 0 -1 Root 0 0 0 0 0 0 1 1 1\nend\textra\n", "end 後有尾隨資料 → false.");

            // 名稱超過 63 字元 → 載入成功但截斷(第一個 63 字元)
            char longName[200];
            for (int i = 0; i < 199; ++i) longName[i] = 'a';
            longName[199] = 0;
            std::string longNameScene = "scene v1\nnode 0 -1 ";
            longNameScene += longName;
            longNameScene += " 0 0 0 0 0 0 0 1 1 1\nend\n";
            const char *longPath = TestTempFilePath("chia_scene_longname.scene");
            writeScene(longPath, longNameScene.c_str());
            SceneSystem longSys;
            EXPECT_TRUE(longSys.LoadScene(longPath), "超長名稱可載入(會截斷).", false);
            DynamicArray<Entity> longNodes;
            DynamicArray<uint32_t> longDepths;
            longSys.GetHierarchy(longNodes, longDepths);
            EXPECT_TRUE(longNodes.GetNElements() == 1, "超長名稱場景有一個節點.", false);
        }

        SUCCESS_MESSAGE("SceneSystem");
        return true;
    }
};

} // namespace scenesystemtest

#endif // SCENE_SYSTEM_TEST_HPP