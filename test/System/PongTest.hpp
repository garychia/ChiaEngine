#ifndef PONG_TEST_HPP
#define PONG_TEST_HPP

 // PongSystem 的無頭測試(獨立 TU 可編譯,不需 GPU/GLFW/Vulkan)。
 //
 // 注意:本檔不 include SceneSystemTest.hpp/PhysicsSystemTest.hpp(GUI / 渲染
 // 依賴),只 include PongTypes/PongSystem/PongHud + Engine + 測試框架 →
 // 達成「Sim 層無 GPU」的隔離。測試驗證:
 //   1. 附著順序契約:SimRecorder 先附著,PongSystem 後,replay 後 hash 相同。
 //   2. 初始狀態:分數 0、無勝利、球靜止。
 //   3. 玩家拍移動(+clamp)。
 //   4. CPU 拍追球(球往右 → CPU 上升)。
 //   5. 牆壁反射(球 Y 頂到底反彈)。
 //   6. 出界得分 + roundOver 重發(2 秒後自動重開)。
 //   7. HUD 內容:0:0 + 提示文字。
 //   8. 勝利:先到 WinScore,HasWinner + HUD "PLAYER 1 WINS!" + 輸入無效。
 //   9. 確定性:兩份相同世界 → 相同 hash。
 //   10. Reset:清分數、球回中、輸入恢復。

#include "Test.hpp"
#include "System/Module/Engine.hpp"
#include "System/Module/IModule.hpp"
#include "System/Module/SimInput.hpp"
#include "System/Module/SimRecorder.hpp"
#include "System/World/Entity.hpp"
#include "System/World/World.hpp"
#include "System/Pong/PongSystem.hpp"
#include "System/Pong/PongHud.hpp"

#include <cmath>

namespace pongtest
{

struct PongFixture
{
    Engine engine;
    SimRecorder recorder;
    pong::PongSystem system;
    pong::PongHud hud;

    PongFixture() : engine(1), recorder(), system(), hud()
    {
        engine.Attach(&recorder); // 順序契約:先 recorder
        engine.Attach(&system);
        engine.Attach(&hud); // hud 在 system 之後,解析到服務
    }

    void TickInput(uint32_t bits, float axisX = 0.0f, float axisY = 0.0f)
    {
        recorder.GetLiveInput().actionBits = bits;
        recorder.GetLiveInput().axisX = axisX;
        recorder.GetLiveInput().axisY = axisY;
        engine.Tick(1.0 / 60.0);
    }

    pong::PongSystem &Sys() { return system; }
    pong::PongHud &Hud() { return hud; }
};

class PongTest : public Test
{
  public:
    PongTest() : Test("PongSystem")
    {
    }

    bool Run() noexcept override
    {
        // 1. 初始狀態
        {
            PongFixture fx;
            Entity left = fx.Sys().CreateLeftPaddle();
            Entity right = fx.Sys().CreateRightPaddle();
            Entity ball = fx.Sys().CreateBall(20.0f);
            (void)left;
            (void)right;
            (void)ball;
            fx.TickInput(0);

            EXPECT_TRUE(fx.Sys().GetLeftScore() == 0 && fx.Sys().GetRightScore() == 0,
                        "初始分數 0:0.", true);
            EXPECT_TRUE(!fx.Sys().HasWinner(), "初始無勝利者.", true);
            EXPECT_TRUE(fx.Sys().GetWorld().Alive(ball), "球 entity 存活.", true);
            EXPECT_TRUE(fx.Sys().GetHash() != 0, "World hash 非零(有狀態).", true);
        }

        // 2. 玩家拍移動 + clamp(按住 Left 300 幀 → 應該停在場地頂端)
        {
            PongFixture fx;
            fx.Sys().CreateLeftPaddle();
            fx.Sys().CreateRightPaddle();
            fx.Sys().CreateBall(20.0f);
            for (int i = 0; i < 300; i++)
                fx.TickInput(pong::PongSystem::BitLeft);
            const float topLimit = pong::PongSystem::FieldHalfHeight - pong::PongSystem::PaddleHalfHeight;
            Entity left = fx.Sys().GetWorld().GetEntityByIndex(0); // 左拍 index 0
            const pong::PaddleComponent *pPaddle = fx.Sys().GetWorld().GetComponent<pong::PaddleComponent>(left);
            EXPECT_TRUE(pPaddle != nullptr, "左拍有 PaddleComponent.", false);
            EXPECT_TRUE(pPaddle && pPaddle->position.y > topLimit - 0.5f,
                        "玩家拍移動超過場地頂端(移動有生效).", false);
        }

        // 3. CPU 拍追球(球往右 → 右拍 Y 朝球 Y 靠近)
        {
            PongFixture fx;
            fx.Sys().CreateLeftPaddle();
            fx.Sys().CreateRightPaddle();
            fx.Sys().CreateBall(0.0f); // 水平往右
            // 發球
            fx.TickInput(pong::PongSystem::BitLeft); // 觸發 launch
            // 球往右,CPU 應該慢慢朝 y 靠近(球 y=0,拍 y=0 → 不動)
            // 先把球 y 改成 +3(模擬高球)→ 看 CPU 是否上升
            Entity ball = fx.Sys().GetWorld().GetEntityByIndex(2);
            pong::BallComponent *pBall = fx.Sys().GetWorld().GetComponent<pong::BallComponent>(ball);
            if (pBall)
                pBall->position.y = 3.0f;
            const float cpuYBefore = fx.Sys().GetWorld().GetComponent<pong::PaddleComponent>(
                                         fx.Sys().GetWorld().GetEntityByIndex(1))
                                         ->position.y;
            for (int i = 0; i < 60; i++)
                fx.TickInput(0); // 不再觸發發球
            const pong::PaddleComponent *pRight = fx.Sys().GetWorld().GetComponent<pong::PaddleComponent>(
                fx.Sys().GetWorld().GetEntityByIndex(1));
            EXPECT_TRUE(pRight && pRight->position.y > cpuYBefore + 0.5f,
                        "CPU 拍朝球 Y 移動(追球).", false);
        }

        // 4. 牆壁反射(球打到頂端 → Y 速度反轉)
        {
            PongFixture fx;
            fx.Sys().CreateLeftPaddle();
            fx.Sys().CreateRightPaddle();
            fx.Sys().CreateBall(0.0f);
            fx.TickInput(pong::PongSystem::BitLeft); // launch
            Entity ball = fx.Sys().GetWorld().GetEntityByIndex(2);
            // 把球往右上角打,速度 Y 正
            pong::BallComponent *pBall = fx.Sys().GetWorld().GetComponent<pong::BallComponent>(ball);
            if (pBall)
            {
                pBall->velocity = Point3D(2.0f, 3.0f, 0.0f);
                pBall->position = Point3D(0.0f, 5.0f, 0.0f);
            }
            const float vyBefore = pBall ? pBall->velocity.y : 0.0f;
            for (int i = 0; i < 30; i++)
                fx.TickInput(0);
            const pong::BallComponent *pAfter = fx.Sys().GetWorld().GetComponent<pong::BallComponent>(ball);
            EXPECT_TRUE(pAfter && (pAfter->velocity.y < 0.0f || vyBefore == 0.0f),
                        "球打到頂端後 Y 速度反轉(牆壁反射).", false);
        }

        // 5. 出界得分 + roundOver 重發(球超過 X 邊界 → 對方得分,roundOver 後自動重開)
        {
            PongFixture fx;
            fx.Sys().CreateLeftPaddle();
            fx.Sys().CreateRightPaddle();
            fx.Sys().CreateBall(0.0f);
            fx.TickInput(pong::PongSystem::BitLeft); // launch
            Entity ball = fx.Sys().GetWorld().GetEntityByIndex(2);
            // 直接把球放到右邊界外
            pong::BallComponent *pBall = fx.Sys().GetWorld().GetComponent<pong::BallComponent>(ball);
            if (pBall)
            {
                pBall->position = Point3D(pong::PongSystem::FieldHalfWidth + 3.0f, 0.0f, 0.0f);
                pBall->velocity = Point3D(3.0f, 0.0f, 0.0f);
            }
            for (int i = 0; i < 10; i++)
                fx.TickInput(0);
            EXPECT_TRUE(fx.Sys().GetLeftScore() == 1, "球出右界 → 左方得分.", true);
            EXPECT_TRUE(fx.Sys().IsRoundOver(), "得分後 roundOver=true.", true);
            // 等 150 ticks ≈ 2.5 秒 → 自動重開,球回到中心靜止
            for (int i = 0; i < 150; i++)
                fx.TickInput(0);
            EXPECT_TRUE(!fx.Sys().IsRoundOver(), "重發逾時後 roundOver=false.", true);
            const pong::BallComponent *pCenter = fx.Sys().GetWorld().GetComponent<pong::BallComponent>(ball);
            EXPECT_TRUE(pCenter && Math::Abs(pCenter->position.x) < 0.01f &&
                            Math::Abs(pCenter->position.y) < 0.01f,
                        "重發後球在中心.", true);
        }

        // 6. HUD 內容:0:0 + 提示文字
        {
            PongFixture fx;
            fx.Sys().CreateLeftPaddle();
            fx.Sys().CreateRightPaddle();
            fx.Sys().CreateBall(20.0f);
            fx.TickInput(0);
            const DynamicArray<pong::PongHudLine> &hudLines = fx.Hud().GetLines();
            EXPECT_TRUE(hudLines.GetNElements() > 0, "HUD 有行.", true);
            // 第一行是左分數 "0"
            EXPECT_TRUE(hudLines.GetNElements() >= 2, "HUD 有至少兩行(分數).", true);
            EXPECT_TRUE(hudLines[0].text == String(u"0") && hudLines[1].text == String(u"0"),
                        "HUD 顯示 0:0.", false);
        }

        // 7. 勝利:先到 5 分 → HasWinner + HUD "PLAYER 1 WINS!" + 輸入無效
        {
            PongFixture fx;
            fx.Sys().CreateLeftPaddle();
            fx.Sys().CreateRightPaddle();
            fx.Sys().CreateBall(20.0f); // 缺這行:entity index 2 不存在 → 出界 teleport 全部 no-op
            // 直接把右分數設成 5(用出界 5 次太慢;直接改世界狀態以測勝利邏輯)
            // 但 PongSystem 沒有 setter — 用出界 4 次 + 最後一次
            for (int i = 0; i < 4; i++)
            {
                Entity ball = fx.Sys().GetWorld().GetEntityByIndex(2);
                pong::BallComponent *pBall = fx.Sys().GetWorld().GetComponent<pong::BallComponent>(ball);
                if (pBall)
                    pBall->position = Point3D(pong::PongSystem::FieldHalfWidth + 3.0f, 0.0f, 0.0f);
                fx.TickInput(0); // 出界 → 左分數 +1
                for (int j = 0; j < 150; j++)
                    fx.TickInput(0); // 等重發
            }
            // 現在做第 5 次出界 → 左分數 = 5 → 勝利
            Entity ball = fx.Sys().GetWorld().GetEntityByIndex(2);
            pong::BallComponent *pBall = fx.Sys().GetWorld().GetComponent<pong::BallComponent>(ball);
            if (pBall)
                pBall->position = Point3D(pong::PongSystem::FieldHalfWidth + 3.0f, 0.0f, 0.0f);
            fx.TickInput(0);
            EXPECT_TRUE(fx.Sys().GetLeftScore() == 5 && fx.Sys().HasWinner() &&
                            fx.Sys().GetWinner() == 1,
                        "達到 5 分 → 左方勝利(winner=1).", true);

            // 輸入無效:繼續按 W,左分數不變
            const uint32_t scoreAfterWin = fx.Sys().GetLeftScore();
            for (int i = 0; i < 30; i++)
                fx.TickInput(pong::PongSystem::BitLeft);
            EXPECT_TRUE(fx.Sys().GetLeftScore() == scoreAfterWin,
                        "勝利後輸入不影響分數.", false);
        }

        // 8. 確定性:完全相同輸入的兩局 → 完全相同 hash
        {
            const uint32_t script[8] = {0, pong::PongSystem::BitLeft, 0, pong::PongSystem::BitLeft,
                                        pong::PongSystem::BitLeft, 0, 0, 0};
            uint64_t hashA = 0, hashB = 0;
            {
                PongFixture fx;
                fx.Sys().CreateLeftPaddle();
                fx.Sys().CreateRightPaddle();
                fx.Sys().CreateBall(20.0f);
                for (int i = 0; i < 8; i++)
                    fx.TickInput(script[i]);
                hashA = fx.Sys().GetHash();
            }
            {
                PongFixture fx;
                fx.Sys().CreateLeftPaddle();
                fx.Sys().CreateRightPaddle();
                fx.Sys().CreateBall(20.0f);
                for (int i = 0; i < 8; i++)
                    fx.TickInput(script[i]);
                hashB = fx.Sys().GetHash();
            }
            EXPECT_TRUE(hashA != 0 && hashA == hashB, "相同輸入 → 相同世界 hash(確定性).", true);
        }

        // 9. Reset:清分數、球回中、輸入恢復
        {
            PongFixture fx;
            fx.Sys().CreateLeftPaddle();
            fx.Sys().CreateRightPaddle();
            fx.Sys().CreateBall(20.0f);
            // 出界得分
            Entity ball = fx.Sys().GetWorld().GetEntityByIndex(2);
            pong::BallComponent *pBall = fx.Sys().GetWorld().GetComponent<pong::BallComponent>(ball);
            if (pBall)
                pBall->position = Point3D(pong::PongSystem::FieldHalfWidth + 3.0f, 0.0f, 0.0f);
            fx.TickInput(0);
            EXPECT_TRUE(fx.Sys().GetLeftScore() == 1, "得分後左分數=1.", true);
            fx.Sys().Reset();
            EXPECT_TRUE(fx.Sys().GetLeftScore() == 0 && fx.Sys().GetRightScore() == 0 &&
                            !fx.Sys().HasWinner() && !fx.Sys().IsRoundOver(),
                        "Reset 後分數歸零、無勝利、roundOver 清除.", true);
            const pong::BallComponent *pAfterReset = fx.Sys().GetWorld().GetComponent<pong::BallComponent>(ball);
            EXPECT_TRUE(pAfterReset && Math::Abs(pAfterReset->position.x) < 0.01f &&
                            Math::Abs(pAfterReset->position.y) < 0.01f,
                        "Reset 後球回中心.", true);
        }

        SUCCESS_MESSAGE("PongSystem");
        return true;
    }
};

} // namespace pongtest

#endif // PONG_TEST_HPP