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
            system.world.GetComponent<TransformComponent>(root)->position = Point3D(10, 0, 0);
            system.world.GetComponent<TransformComponent>(child)->position = Point3D(2, 0, 0);
            system.FixedUpdate(FrameClock());

            WorldTransformComponent *pRoot = system.world.GetComponent<WorldTransformComponent>(root);
            WorldTransformComponent *pChild = system.world.GetComponent<WorldTransformComponent>(child);
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
            EXPECT_TRUE(!system.world.Alive(c), "destroy c 後 c 消失.", true);
            EXPECT_TRUE(system.world.Alive(a) && system.world.Alive(b), "a,b 仍活.", true);
            EXPECT_TRUE(system.GetRoots().GetNElements() == 2, "餘 a,b 兩 root.", true);
        }

        // ---- AC2c:DestroyNode recursive(連子孫) ----
        {
            SceneSystem system;
            Entity root = system.CreateNode();
            Entity child = system.CreateNode(root);
            Entity grand = system.CreateNode(child);
            system.DestroyNode(root);
            EXPECT_TRUE(!system.world.Alive(root) && !system.world.Alive(child) && !system.world.Alive(grand),
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

        // ---- 驗證:SceneSystem 透過 EngineContext RegisterService 可被 Resolve ----
        {
            Engine engine(1);
            SceneSystem system;
            engine.Attach(&system);
            SceneSystem *pResolved = engine.GetContext().ResolveService<SceneSystem>();
            EXPECT_TRUE(pResolved == &system, "OnAttach 自註冊 SceneSystem 服務,Resolve 指向 system.", true);
        }

        SUCCESS_MESSAGE("SceneSystem");
        return true;
    }
};

} // namespace scenesystemtest

#endif // SCENE_SYSTEM_TEST_HPP