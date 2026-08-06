#ifndef PHYSICS_SYSTEM_TEST_HPP
#define PHYSICS_SYSTEM_TEST_HPP

#include "Test.hpp"
#include "Physics/ColliderComponent.hpp"
#include "Physics/ColliderHandler.hpp"
#include "Physics/PhysicsSystem.hpp"
#include "System/Module/Engine.hpp"
#include "System/Module/IModule.hpp"

// PhysicsSystem 整合測試:World + 模組 + 事件廣播 + 確定性。
// 驗證 AC2/AC3/AC4(無重複、無漏、可 replay hash 比對)。
namespace physicssystemtest
{

ColliderComponent MakeAABB(float x, float y, float z, float hx, float hy, float hz)
{
    ColliderComponent c;
    c.kind = ColliderComponent::Kind::AABB;
    c.data[0] = x;
    c.data[1] = y;
    c.data[2] = z;
    c.data[3] = hx;
    c.data[4] = hy;
    c.data[5] = hz;
    return c;
}

ColliderComponent MakeSphere(float x, float y, float z, float radius)
{
    ColliderComponent c;
    c.kind = ColliderComponent::Kind::Sphere;
    c.data[0] = x;
    c.data[1] = y;
    c.data[2] = z;
    c.data[3] = radius;
    return c;
}

// 訂閱探針:逐 tick 記錄收到的碰撞事件序列(依固定順序 append)。
class CollisionRecorder : public IModule
{
  public:
    DynamicArray<OnCollision> received;

    void OnAttach(EngineContext &context) override
    {
        PhysicsSystem *pSystem = context.ResolveService<PhysicsSystem>();
        pSystem->onCollision.Subscribe(this, &CollisionRecorder::OnCollisionEvent);
    }

    void OnDetach(EngineContext &context) override
    {
        PhysicsSystem *pSystem = context.ResolveService<PhysicsSystem>();
        pSystem->onCollision.Unsubscribe(this);
    }

    void OnCollisionEvent(OnCollision event)
    {
        received.Append(event);
    }

    void FixedUpdate(const FrameClock &clock) override
    {
        (void)clock;
    }
};

// 兩段事件序列皆含標準層:UnorderedHash(a,b) == UnorderedHash(c,d)
// API:Entity::GetRaw() 揭露出完整的 handle(含世代)。((a+b)^(a*C)) 的形式是
// 平移之下不變的標準化:同一對{a,b}兩個方向都得到相同值;不同對則(極高機率)不同。
uint64_t UnorderedHash(Entity a, Entity b)
{
    return static_cast<uint64_t>(a.GetRaw()) * 36028797018963968ULL + static_cast<uint64_t>(b.GetRaw()) *
                                                                           360287970189639680ULL +
           static_cast<uint64_t>(a.GetRaw()) + static_cast<uint64_t>(b.GetRaw());
}

class PhysicsSystemTest : public Test
{
  public:
    PhysicsSystemTest() : Test("PhysicsSystem")
    {
    }

    bool Run() noexcept override
    {
        DynamicArray<OnCollision> sequenceA;
        DynamicArray<OnCollision> sequenceB;
        {
            Engine engine(1);
            PhysicsSystem system;
            CollisionRecorder recorder;
            engine.Attach(&system);
            engine.Attach(&recorder);

            // 3 個重疊對 + 1 個孤兒(world space 固定座標):
            // AABB(0) 與 sphere(1) 重疊、AABB(0) 與 sphere(5) 分離、
            // aabb(覆盖) 與 entity-2 sphere 分離 → 恰好 3 對
            Entity e0 = system.world.CreateEntity();
            Entity e1 = system.world.CreateEntity();
            Entity e2 = system.world.CreateEntity();

            system.world.AddComponent<ColliderComponent>(e0, MakeAABB(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f));
            system.world.AddComponent<ColliderComponent>(e1, MakeSphere(0.0f, 0.0f, 0.0f, 2.0f));
            system.world.AddComponent<ColliderComponent>(e2, MakeSphere(5.0f, 5.0f, 5.0f, 1.0f));

            engine.Tick(1.0 / 60.0);
            engine.Tick(1.0 / 60.0);

            EXPECT_TRUE(recorder.received.GetNElements() == 2, "兩個 tick 各發一對(AC3:無漏無重複).", true);
            sequenceA.Append(recorder.received[0]);
            sequenceA.Append(recorder.received[1]);
        }
        {
            // 重播:全新 Engine/PhysicsSystem/recorder,相同輸入
            Engine engine(1);
            PhysicsSystem system;
            CollisionRecorder recorder;
            engine.Attach(&system);
            engine.Attach(&recorder);

            Entity e0 = system.world.CreateEntity();
            Entity e1 = system.world.CreateEntity();
            Entity e2 = system.world.CreateEntity();

            system.world.AddComponent<ColliderComponent>(e0, MakeAABB(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f));
            system.world.AddComponent<ColliderComponent>(e1, MakeSphere(0.0f, 0.0f, 0.0f, 2.0f));
            system.world.AddComponent<ColliderComponent>(e2, MakeSphere(5.0f, 5.0f, 5.0f, 1.0f));

            engine.Tick(1.0 / 60.0);
            engine.Tick(1.0 / 60.0);

            sequenceB.Append(recorder.received[0]);
            sequenceB.Append(recorder.received[1]);

            // 確定性:每 tick 的事件標準層相同 + 最終 World hash 相同(AC4)。
            EXPECT_TRUE(UnorderedHash(sequenceA[0].a, sequenceA[0].b) ==
                            UnorderedHash(sequenceB[0].a, sequenceB[0].b) &&
                        UnorderedHash(sequenceA[1].a, sequenceA[1].b) ==
                            UnorderedHash(sequenceB[1].a, sequenceB[1].b),
                        "碰撞序列(標準層)相同 → 確定性(AC4).", true);
            EXPECT_TRUE(system.world.Hash() != 0, "World hash 非零(有內容).", true);
        }

        // --- 標準化順序 / canonical order 檢查 ---
        {
            Engine engine(1);
            PhysicsSystem system;
            CollisionRecorder recorder;
            engine.Attach(&system);
            engine.Attach(&recorder);

            Entity a = system.world.CreateEntity();
            Entity b = system.world.CreateEntity();
            Entity c = system.world.CreateEntity();

            system.world.AddComponent<ColliderComponent>(a, MakeAABB(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f));
            system.world.AddComponent<ColliderComponent>(b, MakeSphere(0.5f, 0.0f, 0.0f, 2.0f)); // 與 a 重疊
            system.world.AddComponent<ColliderComponent>(c, MakeSphere(0.5f, 0.0f, 0.0f, 2.0f)); // 與 a/b 都重疊

            engine.Tick(1.0 / 60.0);

            EXPECT_TRUE(recorder.received.GetNElements() == 3, "三對(canonical)都收到,無重複無漏.", true);
            // 每個事件都是{a,b}標準層:第一個欄位 index 最小
            bool canonical = true;
            for (size_t i = 0; i < recorder.received.GetNElements(); i++)
            {
                if (recorder.received[i].a.GetIndex() > recorder.received[i].b.GetIndex())
                {
                    canonical = false;
                }
            }
            EXPECT_TRUE(canonical, "所有事件的 a.index <= b.index(無重複對、無 self-pair).", true);
        }

        // --- detach 退訂 ---
        {
            Engine engine(1);
            PhysicsSystem system;
            CollisionRecorder recorder;
            engine.Attach(&system);
            engine.Attach(&recorder);

            Entity a = system.world.CreateEntity();
            Entity b = system.world.CreateEntity();
            system.world.AddComponent<ColliderComponent>(a, MakeSphere(0.0f, 0.0f, 0.0f, 1.0f));
            system.world.AddComponent<ColliderComponent>(b, MakeSphere(1.0f, 0.0f, 0.0f, 1.0f)); // 相切

            engine.Detach(&recorder); // 先退訂再 tick → 不該收到
            engine.Tick(1.0 / 60.0);
            EXPECT_TRUE(recorder.received.IsEmpty(), "Detach 後不再收到事件.", true);
        }

        SUCCESS_MESSAGE("PhysicsSystem");
        return true;
    }
};

} // namespace physicssystemtest

#endif // PHYSICS_SYSTEM_TEST_HPP
