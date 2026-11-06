#ifndef ASTEROID_TEST_HPP
#define ASTEROID_TEST_HPP

#include "System/Asteroid/AsteroidSystem.hpp"
#include "System/Asteroid/AsteroidTypes.hpp"
#include "System/Module/FrameClock.hpp"
#include "Test.hpp"

#include <cmath>
#include <cstdio>

namespace asteroid_test
{

// AsteroidTest:A5.2 Asteroids Sim 層契約。
// 1. 子弹击中大型小行星 → 分裂成 2 颗中型 + 得分
// 2. 子弹击中小型小行星 → 消灭 + 小行星剩余减少
// 3. 飞船与小行星碰撞 → 扣生命 + 飞船重置
// 4. 屏幕包围:超出边界从另一边出现
class AsteroidTest : public Test
{
  public:
    AsteroidTest() : Test("Asteroid")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:子弹击中大型小行星 → 分裂 + 得分 ----
        {
            asteroid::AsteroidSystem sys;
            sys.Reset();

            uint32_t asteroidsBefore = sys.GetAsteroidsRemaining();
            uint32_t scoreBefore = sys.GetScore();

            // 找到第一颗大型小行星,在它上面放一颗子弹
            auto &world = sys.GetWorld();
            auto asteroidView = world.View<asteroid::AsteroidComponent,
                                         asteroid::AsteroidTag>();
            bool hit = false;
            for (auto [ae, ast, aTag] : asteroidView)
            {
                (void)aTag;
                if (!ast->alive || ast->size != 3)
                    continue;
                // 在小行星位置放子弹,ttl 足够
                sys.CreateBullet(ast->position, Point3D(0, 0, 0));
                hit = true;
                break;
            }
            EXPECT_TRUE(hit, "找到大型小行星并放置子弹", true);

            // 跑几 tick 处理碰撞
            for (int i = 0; i < 5; i++)
                sys.FixedUpdate(FrameClock{0, 0, 1.0 / 60.0, 1.0 / 60.0});

            EXPECT_TRUE(sys.GetScore() > scoreBefore, "击中大型小行星 → 得分增加", true);
            // 大型(1) → 2 中型:总数 +1
            EXPECT_TRUE(sys.GetAsteroidsRemaining() == asteroidsBefore + 1,
                        "大型小行星分裂成 2 颗中型 → 总数 +1", true);
        }

        // ---- AC2:子弹击中小型小行星 → 消灭 ----
        {
            asteroid::AsteroidSystem sys;
            sys.Reset();

            // 先击毁一些小行星直到出现小型,或者直接创建一个小型
            // 简化:直接在 World 中创建一个小型小行星
            Entity smallAst = sys.CreateAsteroid(Point3D(0, 0, 0),
                                                 Point3D(0, 0, 0),
                                                 1);
            uint32_t asteroidsBefore = sys.GetAsteroidsRemaining();

            // 在小行星位置放子弹
            sys.CreateBullet(Point3D(0, 0, 0), Point3D(0, 0, 0));

            // 跑几 tick
            for (int i = 0; i < 5; i++)
                sys.FixedUpdate(FrameClock{0, 0, 1.0 / 60.0, 1.0 / 60.0});

            EXPECT_TRUE(sys.GetAsteroidsRemaining() == asteroidsBefore - 1,
                        "小型小行星被击中 → 消灭,剩余 -1", true);
        }

        // ---- AC3:飞船与小行星碰撞 → 扣生命 ----
        {
            asteroid::AsteroidSystem sys;
            sys.Reset();

            uint32_t livesBefore = sys.GetLives();

            // 找到飞船,在飞船位置放一个小行星
            auto &world = sys.GetWorld();
            auto shipView = world.View<asteroid::ShipComponent, asteroid::ShipTag>();
            for (auto [se, ship, sTag] : shipView)
            {
                (void)sTag;
                // 在飞船位置放一个小行星(会立即碰撞)
                sys.CreateAsteroid(ship->position, Point3D(0, 0, 0),
                                   1);
                break;
            }

            // 跑一个 tick,处理碰撞
            sys.FixedUpdate(FrameClock{0, 0, 1.0 / 60.0, 1.0 / 60.0});

            EXPECT_TRUE(sys.GetLives() == livesBefore - 1,
                        "飞船与小行星碰撞 → 生命 -1", true);
        }

        // ---- AC4:屏幕包围 ----
        {
            asteroid::AsteroidSystem sys;
            sys.Reset();

            // 找到一个小行星,设置位置到右边界外
            auto &world = sys.GetWorld();
            auto asteroidView = world.View<asteroid::AsteroidComponent,
                                         asteroid::AsteroidTag>();
            for (auto [ae, ast, aTag] : asteroidView)
            {
                (void)aTag;
                if (!ast->alive)
                    continue;
                // 设置到右边界之外
                ast->position.x = asteroid::AsteroidSystem::FieldHalfWidth + 5.0f;
                ast->position.y = 0.f;
                break;
            }

            // 跑一个 tick,应该包围到左边界
            sys.FixedUpdate(FrameClock{0, 0, 1.0 / 60.0, 1.0 / 60.0});

            // 查找那个小行星,验证位置包围
            bool wrapped = false;
            auto view2 = world.View<asteroid::AsteroidComponent,
                                    asteroid::AsteroidTag>();
            for (auto [ae2, ast2, aTag2] : view2)
            {
                (void)aTag2;
                if (!ast2->alive)
                    continue;
                if (ast2->position.x < 0.f &&
                    ast2->position.x >
                        -asteroid::AsteroidSystem::FieldHalfWidth)
                {
                    wrapped = true;
                    break;
                }
            }
            EXPECT_TRUE(wrapped, "小行星超出右边界 → 从左边界出现", true);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }

  private:
};

} // namespace asteroid_test

#endif // ASTEROID_TEST_HPP
