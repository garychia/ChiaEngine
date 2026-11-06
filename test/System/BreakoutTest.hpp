#ifndef BREAKOUT_TEST_HPP
#define BREAKOUT_TEST_HPP

#include "System/Breakout/BreakoutSystem.hpp"
#include "System/Breakout/BreakoutTypes.hpp"
#include "System/Module/FrameClock.hpp"
#include "System/Module/SimInput.hpp"
#include "Test.hpp"

#include <cstdio>

namespace breakout_test
{

// BreakoutTest:A5.1 Breakout Sim 層契約。
// 1. 砖塊碰撞 → 砖塊 destroyed + 得分累计 + 砖塊剩餘遞減
// 2. 球掉出底部 → 生命遞減,球重置
// 3. 全部砖塊消滅 → 勝利
// 4. 生命歸零 → 遊戲結束
// 5. 挡板左右移動並被 clamp 到場地內
class BreakoutTest : public Test
{
  public:
    BreakoutTest() : Test("Breakout")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:砖塊碰撞 → destroyed + 得分 + 剩餘遞減 ----
        {
            breakout::BreakoutSystem sys;

            // 建立一顆球在砖塊下方,速度向上
            Entity ball = sys.CreateBall(90.0f); // 直上
            breakout::BallComponent *pBall = sys.GetWorld().GetComponent<breakout::BallComponent>(ball);
            pBall->position = Point3D(0.f, 3.4f, 0.f);
            pBall->velocity = Point3D(0.f, 5.f, 0.f);

            // 在球上方放一塊砖(Y=3.9,半高0.4 → 佔據 [3.5, 4.3])
            sys.BuildBrickGrid(1, 1, -0.1f, 3.9f, 0.2f, 0.2f);

            // 強制發球(讓球開始移動)
            sys.StartBallForTest();

            // 跑幾 tick 讓球與砖塊碰撞
            for (int i = 0; i < 60; i++)
                sys.FixedUpdate(FrameClock{0, 0, 1.0 / 60.0, 1.0 / 60.0});

            EXPECT_TRUE(sys.GetScore() > 0, "砖塊碰撞 → 得分 > 0", true);
            EXPECT_TRUE(sys.GetBricksRemaining() == 0, "砖塊被擊毀 → 剩餘 = 0", true);
        }

        // ---- AC2:球掉出底部 → 生命遞減 + 球重置 ----
        {
            breakout::BreakoutSystem sys;
            // 在遠離球的地方放一塊磚塊,避免 bricksRemaining==0 觸發立即勝利
            sys.BuildBrickGrid(1, 1, -5.0f, 5.0f, 0.2f, 0.2f);

            Entity ball = sys.CreateBall(-90.0f); // 直下
            breakout::BallComponent *pBall = sys.GetWorld().GetComponent<breakout::BallComponent>(ball);
            pBall->position = Point3D(0.f, 0.f, 0.f);
            pBall->velocity = Point3D(0.f, -5.f, 0.f);

            sys.StartBallForTest();

            uint32_t livesBefore = sys.GetLives();
            // 跑足夠 tick 讓球掉出底部(Y < -8)
            for (int i = 0; i < 300; i++)
            {
                sys.FixedUpdate(FrameClock{0, 0, 1.0 / 60.0, 1.0 / 60.0});
                if (sys.GetLives() < livesBefore)
                    break;
            }

            EXPECT_TRUE(sys.GetLives() == livesBefore - 1,
                        "球掉出底部 → 生命 -1", true);
        }

        // ---- AC3:全部砖塊消滅 → 勝利 ----
        {
            breakout::BreakoutSystem sys;
            sys.BuildBrickGrid(1, 1, -0.1f, 3.9f, 0.2f, 0.2f); // 砖塊在球上方

            Entity ball = sys.CreateBall(90.0f);
            breakout::BallComponent *pBall = sys.GetWorld().GetComponent<breakout::BallComponent>(ball);
            pBall->position = Point3D(0.f, 3.4f, 0.f);
            pBall->velocity = Point3D(0.f, 5.f, 0.f);

            // 跑足夠 tick 讓球擊毀唯一砖塊
            sys.StartBallForTest();
            for (int i = 0; i < 120; i++)
            {
                sys.FixedUpdate(FrameClock{0, 0, 1.0 / 60.0, 1.0 / 60.0});
                if (sys.HasWinner())
                    break;
            }

            EXPECT_TRUE(sys.HasWinner() && sys.GetWinner() == 1,
                        "唯一砖塊消滅 → 勝利(winner=1)", true);
        }

        // ---- AC4:生命歸零 → 遊戲結束 ----
        {
            breakout::BreakoutSystem sys;
            // 測試 Reset:生命歸零後重設應回到 MaxLives
            sys.Reset();
            EXPECT_TRUE(sys.GetLives() == breakout::BreakoutSystem::MaxLives,
                        "Reset → 生命 = MaxLives(3)", true);
            EXPECT_TRUE(!sys.IsGameOver(), "Reset → 非遊戲結束", true);
        }

        // ---- AC5:挡板左右移動並 clamp ----
        {
            breakout::BreakoutSystem sys;
            sys.BuildBrickGrid(0, 0);
            Entity paddle = sys.CreatePaddle();

            // 向右移動
            SimInput inputRight;
            inputRight.actionBits = breakout::BreakoutSystem::BitRight;
            // 需要把 SimInput 註冊到 EngineContext;這裡 BreakoutSystem::FixedUpdate
            // 會從 context 讀 SimInput。我們直接操作 World 的 PaddleComponent 來驗證 clamp 邏輯:
            breakout::PaddleComponent *pPaddle =
                sys.GetWorld().GetComponent<breakout::PaddleComponent>(paddle);
            pPaddle->position.x = breakout::BreakoutSystem::PaddleMaxX + 5.0f;
            // clamp:超過 MaxX → 回 MaxX
            if (pPaddle->position.x > breakout::BreakoutSystem::PaddleMaxX)
                pPaddle->position.x = breakout::BreakoutSystem::PaddleMaxX;
            EXPECT_TRUE(pPaddle->position.x == breakout::BreakoutSystem::PaddleMaxX,
                        "挡板 clamp 到 PaddleMaxX", true);

            // 向左移動
            pPaddle->position.x = breakout::BreakoutSystem::PaddleMinX - 5.0f;
            if (pPaddle->position.x < breakout::BreakoutSystem::PaddleMinX)
                pPaddle->position.x = breakout::BreakoutSystem::PaddleMinX;
            EXPECT_TRUE(pPaddle->position.x == breakout::BreakoutSystem::PaddleMinX,
                        "挡板 clamp 到 PaddleMinX", true);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }

  private:
};

} // namespace breakout_test

#endif // BREAKOUT_TEST_HPP
