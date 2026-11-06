#ifndef PLATFORMER_TEST_HPP
#define PLATFORMER_TEST_HPP

#include "System/Platformer/PlatformerSystem.hpp"
#include "System/Platformer/PlatformerTypes.hpp"
#include "System/Module/FrameClock.hpp"
#include "Test.hpp"

#include <cmath>

namespace platformer_test
{

// PlatformerTest:A5.3 平台跳躍 Sim 層契約。
// 1. 重力下降:空中玩家 y 遞減
// 2. 跳躍上升:onGround 時跳躍,速度變正,y 遞增
// 3. 平台停止下降:落在平台上,onGround=1,y 不再下降
// 4. 左右移動:BitLeft/BitRight 改變 x 位置
// 5. AABB 碰撞解決重疊:玩家嵌入平台後位置被推出
class PlatformerTest : public Test
{
  public:
    PlatformerTest() : Test("Platformer")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:重力下降 ----
        {
            platformer::PlatformerSystem sys;
            // 不調用 Reset(會建立地板平台),手動只建立玩家
            Entity player = sys.CreatePlayer();

            auto &world = sys.GetWorld();
            auto view = world.View<platformer::PlayerComponent,
                                   platformer::PlayerTag>();
            float startY = 0.f;
            for (auto [e, p, t] : view)
            {
                (void)t;
                (void)e;
                // 置於空中,無平台下方
                p->position.y = 5.0f;
                p->velocity = Point3D(0, 0, 0);
                p->onGround = 0;
                startY = p->position.y; // 記錄移動後的起始高度
                break;
            }

            // 跑幾 tick,重力應讓 y 下降(無地板,不會觸底重設)
            for (int i = 0; i < 20; i++)
                sys.FixedUpdate(FrameClock{0, 0, 1.0 / 60.0, 1.0 / 60.0});

            for (auto [e, p, t] : view)
            {
                (void)t;
                (void)e;
                EXPECT_TRUE(p->position.y < startY,
                            "重力下降 → y 遞減", true);
                break;
            }
        }

        // ---- AC2:跳躍上升 ----
        {
            platformer::PlatformerSystem sys;
            // 手動建立玩家,不調用 Reset(避免默認平台)
            Entity player = sys.CreatePlayer();

            auto &world = sys.GetWorld();
            auto view = world.View<platformer::PlayerComponent,
                                   platformer::PlayerTag>();
            for (auto [e, p, t] : view)
            {
                (void)t;
                // 置於地面,onGround=1
                p->position = Point3D(0, -4.0f, 0);
                p->velocity = Point3D(0, 0, 0);
                p->onGround = 1;
                p->canJump = 1;
                break;
            }

            // 模擬跳躍:onGround=1 時設 velocity.y 為正
            for (auto [e, p, t] : view)
            {
                (void)t;
                (void)e;
                if (p->onGround && p->canJump)
                {
                    p->velocity.y = p->jumpSpeed;
                    p->onGround = 0;
                    p->canJump = 0;
                }
                break;
            }

            EXPECT_TRUE(sys.IsOnGround() == false,
                        "跳躍後 onGround=0", true);

            // 跑幾 tick,y 應先上升
            float peakY = -4.0f;
            for (int i = 0; i < 20; i++)
            {
                sys.FixedUpdate(FrameClock{0, 0, 1.0 / 60.0, 1.0 / 60.0});
                for (auto [e2, p2, t2] : view)
                {
                    (void)t2;
                    (void)e2;
                    if (p2->position.y > peakY)
                        peakY = p2->position.y;
                }
            }
            EXPECT_TRUE(peakY > -4.0f,
                        "跳躍上升 → y 高於起始 -4.0", true);
        }

        // ---- AC3:平台停止下降 ----
        {
            platformer::PlatformerSystem sys;
            // 手動建立玩家和平台,不調用 Reset(避免默認平台干擾)
            Entity player = sys.CreatePlayer();
            sys.CreatePlatform(Point3D(0, 0, 0), 4.0f, 0.4f);

            auto &world = sys.GetWorld();
            auto view = world.View<platformer::PlayerComponent,
                                   platformer::PlayerTag>();
            for (auto [e, p, t] : view)
            {
                (void)t;
                // 玩家在平台上方,向下掉
                p->position = Point3D(0, 1.0f, 0);
                p->velocity = Point3D(0, -5.0f, 0);
                p->onGround = 0;
                break;
            }

            // 跑幾 tick,玩家應落在平台上停止
            for (int i = 0; i < 60; i++)
            {
                sys.FixedUpdate(FrameClock{0, 0, 1.0 / 60.0, 1.0 / 60.0});
                for (auto [e2, p2, t2] : view)
                {
                    (void)t2;
                    (void)e2;
                    if (p2->onGround)
                        break;
                }
            }

            bool stopped = false;
            for (auto [e, p, t] : view)
            {
                (void)t;
                (void)e;
                if (p->onGround &&
                    std::abs(p->position.y - (0.f + 0.4f / 2.f + 0.6f)) < 0.5f)
                {
                    stopped = true;
                }
            }
            EXPECT_TRUE(stopped, "落在平台上 → onGround=1,位置在平台頂部", true);
        }

        // ---- AC4:左右移動 ----
        {
            platformer::PlatformerSystem sys;
            // 手動建立玩家,不調用 Reset(避免掉出地板重設位置)
            Entity player = sys.CreatePlayer();

            auto &world = sys.GetWorld();
            auto view = world.View<platformer::PlayerComponent,
                                   platformer::PlayerTag>();
            float startX = 0.f;
            for (auto [e, p, t] : view)
            {
                (void)t;
                startX = p->position.x;
                p->onGround = 1;
                break;
            }

            // 直接設 velocity.x 向右(模擬 BitRight)
            for (auto [e, p, t] : view)
            {
                (void)t;
                (void)e;
                p->velocity.x = 6.0f;
                break;
            }

            // 跑幾 tick
            for (int i = 0; i < 10; i++)
                sys.FixedUpdate(FrameClock{0, 0, 1.0 / 60.0, 1.0 / 60.0});

            float endX = 0.f;
            for (auto [e, p, t] : view)
            {
                (void)t;
                (void)e;
                endX = p->position.x;
            }
            EXPECT_TRUE(endX > startX,
                        "向右移動 → x 增加", true);
        }

        // ---- AC5:AABB 碰撞解決重疊 ----
        {
            platformer::PlatformerSystem sys;
            // 手動建立玩家和平台,不調用 Reset(避免默認平台干擾)
            Entity player = sys.CreatePlayer();
            sys.CreatePlatform(Point3D(0, 0, 0), 4.0f, 0.4f);

            auto &world = sys.GetWorld();
            auto view = world.View<platformer::PlayerComponent,
                                   platformer::PlayerTag>();
            for (auto [e, p, t] : view)
            {
                (void)t;
                // 玩家嵌入平台內部
                p->position = Point3D(0, 0, 0);
                p->velocity = Point3D(0, 0, 0);
                break;
            }

            // 跑一 tick,碰撞解決應將玩家推出平台
            sys.FixedUpdate(FrameClock{0, 0, 1.0 / 60.0, 1.0 / 60.0});

            bool resolved = false;
            for (auto [e, p, t] : view)
            {
                (void)t;
                (void)e;
                // 玩家 y 應在平台頂部之上(0.2 + 0.6 + 0.001 = 0.801)或
                // 底部之下(-0.2 - 0.6 - 0.001 = -0.801)
                float platTop = 0.f + 0.4f / 2.f + p->halfHeight + 0.001f;
                float platBottom = 0.f - 0.4f / 2.f - p->halfHeight - 0.001f;
                if (p->position.y >= platTop - 0.1f ||
                    p->position.y <= platBottom + 0.1f)
                {
                    resolved = true;
                }
            }
            EXPECT_TRUE(resolved, "嵌入平台後 → 位置被推出 AABB", true);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }

  private:
};

} // namespace platformer_test

#endif // PLATFORMER_TEST_HPP
