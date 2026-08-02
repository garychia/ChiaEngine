#ifndef SYSTEM_WORLD_TEST_HPP
#define SYSTEM_WORLD_TEST_HPP

#include "Test.hpp"
#include "System/Module/Engine.hpp"
#include "System/Module/IModule.hpp"
#include "System/World/World.hpp"

#include <cmath>

// ---------------- 測試元件 ----------------

struct Position
{
    float x, y, z;
};

struct Velocity
{
    float x, y, z;
};

struct Health
{
    int hp;
};

// ---------------- 測試系統:移動系統(確定性) ----------------

class MoveSystem : public IModule
{
  public:
    World world;

    void FixedUpdate(const FrameClock &clock) override
    {
        const float dt = static_cast<float>(clock.fixedDeltaSeconds);
        for (auto [entity, pos, vel] : world.View<Position, Velocity>())
        {
            (void)entity;
            pos->x += vel->x * dt;
            pos->y += vel->y * dt;
            pos->z += vel->z * dt;
        }
    }
};

// ---------------- 測試主體 ----------------

class WorldTest : public Test
{
  public:
    WorldTest() : Test("SystemWorld")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("Entity create and alive");
        {
            World world;
            Entity a = world.CreateEntity();
            Entity b = world.CreateEntity();
            EXPECT_TRUE(world.Alive(a), "建立的 entity 是活的.", true);
            EXPECT_TRUE(world.Alive(b), "建立的 entity 是活的.", true);
            EXPECT_TRUE(a.GetIndex() != b.GetIndex(), "不同 entity 不同索引.", true);
            EXPECT_TRUE(world.GetEntityCount() == 2, "數量正確.", true);
        }

        TEST_MESSAGE("Entity destroy and generation reuse");
        {
            World world;
            Entity a = world.CreateEntity();
            const uint32_t aGen = a.GetGeneration();
            world.DestroyEntity(a);
            EXPECT_TRUE(!world.Alive(a), "銷毀後 handle 失效.", true);
            EXPECT_TRUE(world.GetEntityCount() == 0, "數量歸零.", true);
            Entity b = world.CreateEntity();
            EXPECT_TRUE(b.GetIndex() == a.GetIndex(), "slot 重用.", true);
            EXPECT_TRUE(b.GetGeneration() == aGen + 1, "世代 +1,舊 handle 永久失效.", true);
            EXPECT_TRUE(!world.Alive(a), "舊 handle 仍然失效.", true);
            EXPECT_TRUE(world.Alive(b), "新 handle 有效.", true);
        }

        TEST_MESSAGE("Component add/get/has/remove");
        {
            World world;
            Entity e = world.CreateEntity();
            EXPECT_TRUE(!world.HasComponent<Position>(e), "初始無元件.", true);
            world.AddComponent<Position>(e, Position{1.0f, 2.0f, 3.0f});
            EXPECT_TRUE(world.HasComponent<Position>(e), "加完有元件.", true);
            Position *pPos = world.GetComponent<Position>(e);
            EXPECT_TRUE(pPos != nullptr, "可取回元件.", true);
            EXPECT_TRUE(pPos->x == 1.0f && pPos->y == 2.0f && pPos->z == 3.0f, "值正確.", true);
            world.RemoveComponent<Position>(e);
            EXPECT_TRUE(!world.HasComponent<Position>(e), "移除後沒有.", true);
            EXPECT_TRUE(world.GetComponent<Position>(e) == nullptr, "移除後取回 nullptr.", true);
        }

        TEST_MESSAGE("Multiple components and isolation");
        {
            World world;
            Entity e1 = world.CreateEntity();
            Entity e2 = world.CreateEntity();
            world.AddComponent<Position>(e1, Position{1.0f, 0.0f, 0.0f});
            world.AddComponent<Health>(e1, Health{100});
            world.AddComponent<Health>(e2, Health{50});
            EXPECT_TRUE(world.HasComponent<Position>(e1) && world.HasComponent<Health>(e1), "e1 兩種元件.", true);
            EXPECT_TRUE(!world.HasComponent<Position>(e2) && world.HasComponent<Health>(e2), "e2 只有 Health.", true);
            EXPECT_TRUE(world.GetComponent<Health>(e2)->hp == 50, "不同 entity 元件隔離.", true);
        }

        TEST_MESSAGE("DestroyEntity removes all components");
        {
            World world;
            Entity e = world.CreateEntity();
            world.AddComponent<Position>(e, Position{0.0f, 0.0f, 0.0f});
            world.AddComponent<Velocity>(e, Velocity{0.0f, 0.0f, 0.0f});
            world.DestroyEntity(e);
            EXPECT_TRUE(!world.Alive(e), "entity 已死.", true);
            EXPECT_TRUE(world.GetComponent<Position>(e) == nullptr, "元件已清.", true);
            EXPECT_TRUE(world.GetComponent<Velocity>(e) == nullptr, "元件已清.", true);
        }

        TEST_MESSAGE("Component swap-remove consistency");
        {
            World world;
            Entity a = world.CreateEntity();
            Entity b = world.CreateEntity();
            Entity c = world.CreateEntity();
            world.AddComponent<Health>(a, Health{10});
            world.AddComponent<Health>(b, Health{20});
            world.AddComponent<Health>(c, Health{30});
            world.RemoveComponent<Health>(b); // 刪中間,swap-remove 把 c 搬過來
            EXPECT_TRUE(world.GetComponent<Health>(a)->hp == 10, "a 仍在.", true);
            EXPECT_TRUE(world.GetComponent<Health>(c)->hp == 30, "c 仍在(搬移後 lookup 正確).", true);
            EXPECT_TRUE(world.GetComponent<Health>(b) == nullptr, "b 已刪.", true);
        }

        TEST_MESSAGE("View filters and mutates");
        {
            World world;
            Entity e1 = world.CreateEntity();
            Entity e2 = world.CreateEntity();
            Entity e3 = world.CreateEntity();
            world.AddComponent<Position>(e1, Position{0.0f, 0.0f, 0.0f});
            world.AddComponent<Velocity>(e1, Velocity{1.0f, 0.0f, 0.0f});
            world.AddComponent<Position>(e2, Position{0.0f, 0.0f, 0.0f});
            world.AddComponent<Position>(e3, Position{0.0f, 0.0f, 0.0f});
            world.AddComponent<Velocity>(e3, Velocity{2.0f, 0.0f, 0.0f});

            uint32_t count = 0;
            for (auto [entity, pos, vel] : world.View<Position, Velocity>())
            {
                (void)vel;
                pos->x += 100.0f; // 透過 View 修改
                count++;
                EXPECT_TRUE(world.Alive(entity), "View 給的是有效 entity(含世代).", true);
            }
            EXPECT_TRUE(count == 2, "只有 e1/e3 有兩個元件.", true);
            EXPECT_TRUE(world.GetComponent<Position>(e1)->x == 100.0f, "e1 被修改.", true);
            EXPECT_TRUE(world.GetComponent<Position>(e2)->x == 0.0f, "e2 沒被碰.", true);
            EXPECT_TRUE(world.GetComponent<Position>(e3)->x == 100.0f, "e3 被修改.", true);

            // 移除 Velocity 後,View 只剩 e1
            world.RemoveComponent<Velocity>(e3);
            count = 0;
            for (auto [entity, pos, vel] : world.View<Position, Velocity>())
            {
                (void)entity;
                (void)pos;
                (void)vel;
                count++;
            }
            EXPECT_TRUE(count == 1, "移除後過濾正確.", true);
        }

        TEST_MESSAGE("View after destroy and slot reuse");
        {
            World world;
            Entity a = world.CreateEntity();
            Entity b = world.CreateEntity();
            world.AddComponent<Position>(a, Position{1.0f, 0.0f, 0.0f});
            world.AddComponent<Position>(b, Position{2.0f, 0.0f, 0.0f});
            world.AddComponent<Velocity>(a, Velocity{0.0f, 1.0f, 0.0f});
            world.AddComponent<Velocity>(b, Velocity{0.0f, 2.0f, 0.0f});
            world.DestroyEntity(a); // 刪 dense 中間/開頭,觸發 swap

            Entity c = world.CreateEntity(); // 重用 a 的 slot
            world.AddComponent<Position>(c, Position{3.0f, 0.0f, 0.0f});
            world.AddComponent<Velocity>(c, Velocity{0.0f, 3.0f, 0.0f});

            uint32_t count = 0;
            float sumY = 0.0f;
            for (auto [entity, pos, vel] : world.View<Position, Velocity>())
            {
                (void)entity;
                (void)pos;
                sumY += vel->y;
                count++;
            }
            EXPECT_TRUE(count == 2, "銷毀+重用後 View 走訪正確.", true);
            EXPECT_TRUE(sumY == 5.0f, "只剩 b(2) 和 c(3).", true);
        }

        TEST_MESSAGE("Deterministic system tick (Engine + World)");
        {
            // 第一次執行
            Engine engine(1); // 每幀 1 個固定步進
            MoveSystem moveSystem;
            engine.Attach(&moveSystem);

            Entity e = moveSystem.world.CreateEntity();
            moveSystem.world.AddComponent<Position>(e, Position{0.0f, 0.0f, 0.0f});
            moveSystem.world.AddComponent<Velocity>(e, Velocity{60.0f, 0.0f, 0.0f});

            engine.Tick(1.0 / 60.0); // 60 * 1/60 = 1 單位/秒
            engine.Tick(1.0 / 60.0);
            const float x1 = moveSystem.world.GetComponent<Position>(e)->x;
            EXPECT_TRUE(std::fabs(x1 - 2.0f) < 1e-4f, "固定步進累加確定.", true);

            // 重播:全新的 system + 相同輸入,結果必須 bit 級相同
            Engine replayEngine(1);
            MoveSystem replaySystem;
            replayEngine.Attach(&replaySystem);

            Entity r = replaySystem.world.CreateEntity();
            replaySystem.world.AddComponent<Position>(r, Position{0.0f, 0.0f, 0.0f});
            replaySystem.world.AddComponent<Velocity>(r, Velocity{60.0f, 0.0f, 0.0f});

            replayEngine.Tick(1.0 / 60.0);
            replayEngine.Tick(1.0 / 60.0);
            const float x2 = replaySystem.world.GetComponent<Position>(r)->x;

            EXPECT_TRUE(std::fabs(x2 - 2.0f) < 1e-4f, "重播結果正確.", true);
            EXPECT_TRUE(x1 == x2, "同輸入 → bit 級相同結果(確定性).", true);
        }

        SUCCESS_MESSAGE("SystemWorld");
        return true;
    }
};

#endif // SYSTEM_WORLD_TEST_HPP
