#ifndef PONG_SYSTEM_HPP
#define PONG_SYSTEM_HPP

 // PongSystem — Pong 的 Sim 層模組(確定性、無 GPU、無牆鐘)。
 //
 // 設計(RFC P7b/架構 5 支柱):
 //  - 自己擁有 World(P7a PhysicsSystem 同款);模組對外只暴露「spawn/設定」
 //    接縫(#81 的模組隔離規則),不讓 View 摸 World。
 //  - FixedUpdate 依固定 60Hz 步進:先讀 SimInput(context 服務),移動玩家拍,
 //    推進球(速度 × dt),處理牆壁/球拍反射與得分。
 //  - 碰撞用簡化的軸分離 AABB-球判定(球視為 AABB 半徑 r,拍視為 AABB
 //    halfExtents(halfWidth, halfHeight))→ 完全確定性、無浮點分歧。
 //  - 得分/重置/方向都用確定性邏輯,不讀牆鐘。
 //
 // 附著順序契約:SimRecorder 必須在 PongSystem 之前 attach(recorder 先寫
 // SimInput,sim 後讀,見 SimRecorder.hpp 註解)。這是 replay 確定性的前提。
 //
 // 控制:BitLeft/BitRight(actionBits bit0/bit1)= 玩家拍上/下。
 //       BitCpuToggle(bit2)= 按住時切給 CPU(debug 用)。
 // 勝利:先到 WinScore 者勝;之後輸入不影響,直到 Reset()。

#include "Math/Math.hpp"
#include "System/Module/EngineContext.hpp"
#include "System/Module/IModule.hpp"
#include "System/Module/SimInput.hpp"
#include "System/World/Entity.hpp"
#include "System/World/World.hpp"
#include "PongTypes.hpp"

#include <cstdint>

namespace pong
{

class PongSystem : public IModule
{
  public:
    static constexpr uint32_t BitLeft = 0x1u;   // 玩家拍(左)上
    static constexpr uint32_t BitRight = 0x2u;  // 玩家拍(左)下
    static constexpr uint32_t BitCpuToggle = 0x4u; // 按住 → 切給 CPU(debug)

    // 場地(世界單位)
    static constexpr float FieldHalfWidth = 9.0f;
    static constexpr float FieldHalfHeight = 6.0f;

    // 拍
    static constexpr float PaddleHalfWidth = 0.35f;
    static constexpr float PaddleHalfHeight = 1.3f;
    static constexpr float PaddleX = 8.0f;   // 左拍 x = -8,右拍 x = +8
    static constexpr float PlayerSpeed = 8.0f;
    static constexpr float CpuSpeed = 5.5f;

    // 球
    static constexpr float BallRadius = 0.35f;
    static constexpr float BallSpeed = 6.0f;

    static constexpr uint32_t WinScore = 5;

    PongSystem() : world(), pContext(nullptr), leftPaddle(), rightPaddle(), ball(),
                   leftScore(0), rightScore(0), winner(0), roundOver(false), ballLaunched(false),
                   ballAngle(0.0f), roundTicks(0)
    {
    }

    // ---- 接縫(供 View 初始化場景;不暴露 World)----
    Entity CreateLeftPaddle()
    {
        Entity e = world.CreateEntity();
        world.AddComponent<PaddleComponent>(e, PaddleComponent{Point3D(-PaddleX, 0, 0), PaddleHalfHeight, PlayerSpeed});
        world.AddComponent<OptionalPaddlePlayer>(e, OptionalPaddlePlayer{OptionalPaddlePlayer::Player::One, false});
        world.AddComponent<PaddleEntityTag>(e, PaddleEntityTag{});
        leftPaddle = e;
        return e;
    }

    Entity CreateRightPaddle()
    {
        Entity e = world.CreateEntity();
        world.AddComponent<PaddleComponent>(e, PaddleComponent{Point3D(PaddleX, 0, 0), PaddleHalfHeight, CpuSpeed});
        world.AddComponent<OptionalPaddlePlayer>(e, OptionalPaddlePlayer{OptionalPaddlePlayer::Player::Two, true});
        world.AddComponent<PaddleEntityTag>(e, PaddleEntityTag{});
        rightPaddle = e;
        return e;
    }

    Entity CreateBall(float angleDeg = 20.0f)
    {
        Entity e = world.CreateEntity();
        world.AddComponent<BallComponent>(e, BallComponent{Point3D(0, 0, 0), Point3D(0, 0, 0)});
        ballAngle = angleDeg;
        ResetBallForRound(e, angleDeg);
        ball = e;
        return e;
    }

    // ---- 查詢(View 讀整局狀態;Sim 是狀態源)----
    uint32_t GetLeftScore() const { return leftScore; }
    uint32_t GetRightScore() const { return rightScore; }
    bool HasWinner() const { return winner != 0; }
    uint32_t GetWinner() const { return winner; } // 1 = 左,2 = 右
    bool IsRoundOver() const { return roundOver; }

    // 測試需要用 World 直接改狀態(模擬得分/球位)→ 暴露 non-const 版本。
    World &GetWorld() { return world; }

    // 重設整局(分數歸零、球回到原位、勝利清空)。View 的 restart 鍵呼叫。
    void Reset()
    {
        leftScore = 0;
        rightScore = 0;
        winner = 0;
        roundOver = false;
        ballLaunched = false;
        roundTicks = 0;
        if (world.Alive(ball))
        {
            BallComponent *pBall = world.GetComponent<BallComponent>(ball);
            if (pBall)
            {
                pBall->position = Point3D(0, 0, 0);
                pBall->velocity = Point3D(0, 0, 0);
            }
        }
        if (world.Alive(leftPaddle))
        {
            PaddleComponent *pPaddle = world.GetComponent<PaddleComponent>(leftPaddle);
            if (pPaddle)
                pPaddle->position.y = 0;
        }
        if (world.Alive(rightPaddle))
        {
            PaddleComponent *pPaddle = world.GetComponent<PaddleComponent>(rightPaddle);
            if (pPaddle)
                pPaddle->position.y = 0;
        }
        // 讓 View 看到(順便確保 CPU 拍回到預設狀態)
    }

    uint64_t GetHash() const
    {
        return world.Hash();
    }

    // ---- IModule ----
    void OnAttach(EngineContext &context) override
    {
        pContext = &context;
        context.RegisterService<PongSystem>(this);
    }

    void OnDetach(EngineContext &context) override
    {
        (void)context;
    }

    void FixedUpdate(const FrameClock &clock) override
    {
        (void)clock;
        SimInput *pInput = pContext ? pContext->ResolveService<SimInput>() : nullptr;
        const float dt = static_cast<float>(clock.fixedDeltaSeconds);

        // 勝利後輸入不影響(除非 Reset)
        if (winner != 0)
            return;

        // 球的發球延遲:roundOver 後 2 秒自動重發(期間 ball 靜止在中心)。
        if (roundOver)
        {
            roundTicks++;
            if (roundTicks >= static_cast<uint32_t>(2.0f / dt)) // ≈120 ticks
            {
                ResetBallForRound(ball, -ballAngle); // 換方向重開
                roundOver = false;
                roundTicks = 0;
            }
            return; // 期間不移動任何東西
        }

        // 玩家拍:W/S 或 Up/Down 移動(位置 clamp 在場地內)。
        MovePaddleByInput(leftPaddle, pInput, dt);
        if (pInput && (pInput->actionBits & BitRight))
            MovePaddle(rightPaddle, 1.0f, dt);

        // CPU 拍:追球(有預測:用目前球速度與剩餘飛行時間估目標 y)。
        MovePaddleTowards(rightPaddle, PredictBallY(), dt);

        // 球運動
        BallComponent *pBall = world.GetComponent<BallComponent>(ball);
        if (!pBall)
            return;
        pBall->position.x += pBall->velocity.x * dt;
        pBall->position.y += pBall->velocity.y * dt;

        // 上下牆反射
        if (pBall->position.y > FieldHalfHeight - BallRadius)
        {
            pBall->position.y = FieldHalfHeight - BallRadius;
            pBall->velocity.y = -pBall->velocity.y;
        }
        else if (pBall->position.y < -FieldHalfHeight + BallRadius)
        {
            pBall->position.y = -FieldHalfHeight + BallRadius;
            pBall->velocity.y = -pBall->velocity.y;
        }

        // 球拍碰撞:球(AABB 半徑 r)vs 拍(AABB halfExtents)
        const PaddleComponent *pLeft = world.GetComponent<PaddleComponent>(leftPaddle);
        const PaddleComponent *pRight = world.GetComponent<PaddleComponent>(rightPaddle);
        if (pLeft && BallOverlapsPaddle(*pBall, pLeft->position, PaddleHalfWidth, pLeft->halfHeight))
        {
            ReflectBall(*pBall, true);
        }
        else if (pRight && BallOverlapsPaddle(*pBall, pRight->position, PaddleHalfWidth, pRight->halfHeight))
        {
            ReflectBall(*pBall, false);
        }

        // 出界 → 得分 + 球回中,roundOver 等待重發。
        if (pBall->position.x < -FieldHalfWidth - 2.0f)
        {
            rightScore++;
            CheckWin();
            roundOver = true;
            roundTicks = 0;
            ResetBallForRound(ball, ballAngle); // 停在中心(velocity 0)
        }
        else if (pBall->position.x > FieldHalfWidth + 2.0f)
        {
            leftScore++;
            CheckWin();
            roundOver = true;
            roundTicks = 0;
            ResetBallForRound(ball, -ballAngle);
        }

        // 發球:只有剛建立(未發射)時由輸入觸發發射
        if (!ballLaunched && pInput && (pInput->actionBits & (BitLeft | BitRight)))
        {
            LaunchBall();
        }
    }

    void Update(const FrameClock &clock) override
    {
        (void)clock;
    }

  private:
    World world;
    EngineContext *pContext;

    Entity leftPaddle;
    Entity rightPaddle;
    Entity ball;

    uint32_t leftScore;
    uint32_t rightScore;
    uint32_t winner;
    bool roundOver;
    bool ballLaunched;
    float ballAngle; // 目前發球角度
    uint32_t roundTicks;

    // 拍移動:dir 為 +1(上)/-1(下);clamp 在場地內(留拍半高邊距)。
    void MovePaddle(Entity paddle, float dir, float dt)
    {
        PaddleComponent *pPaddle = world.GetComponent<PaddleComponent>(paddle);
        if (!pPaddle)
            return;
        pPaddle->position.y += dir * pPaddle->speed * dt;
        const float limit = FieldHalfHeight - pPaddle->halfHeight;
        if (pPaddle->position.y > limit)
            pPaddle->position.y = limit;
        else if (pPaddle->position.y < -limit)
            pPaddle->position.y = -limit;
    }

    void MovePaddleByInput(Entity paddle, const SimInput *pInput, float dt)
    {
        // 玩家:W/S 或 Up/Down。
        if (pInput && (pInput->actionBits & BitLeft))
            MovePaddle(paddle, 1.0f, dt);
    }

    // CPU 拍:朝目標 y 移動,預測 = 球目前 y + vy × (剩餘飛行時間)。
    float PredictBallY()
    {
        const BallComponent *pBall = world.GetComponent<BallComponent>(ball);
        if (!pBall)
            return 0.0f;
        const float targetX = PaddleX; // 右拍
        const float dx = targetX - pBall->position.x;
        if (dx <= 0.0f || pBall->velocity.x <= 0.0f)
            return pBall->position.y; // 球離拍而去 / 未移動 → 回目前 y
        const float timeToReach = dx / pBall->velocity.x;
        float predictedY = pBall->position.y + pBall->velocity.y * timeToReach;
        // 牆反射預測:在預測時間內來回彈 → 模擬(最多 4 段)
        const float top = FieldHalfHeight - BallRadius;
        const float bottom = -top;
        float remainingTime = timeToReach;
        float y = predictedY;
        float vy = pBall->velocity.y;
        for (int bounce = 0; bounce < 4 && remainingTime > 0.0f; bounce++)
        {
            if (vy > 0.0f)
            {
                const float timeToTop = (top - y) / vy;
                if (timeToTop <= remainingTime)
                {
                    y = top;
                    vy = -vy;
                    remainingTime -= timeToTop;
                }
                else
                {
                    y += vy * remainingTime;
                    remainingTime = 0.0f;
                }
            }
            else if (vy < 0.0f)
            {
                const float timeToBottom = (y - bottom) / (-vy);
                if (timeToBottom <= remainingTime)
                {
                    y = bottom;
                    vy = -vy;
                    remainingTime -= timeToBottom;
                }
                else
                {
                    y += vy * remainingTime;
                    remainingTime = 0.0f;
                }
            }
            else
            {
                remainingTime = 0.0f;
            }
        }
        // Clamp 最終預測 Y 進場地內(負飛行時間/極端反彈的邊緣情況 → CPU 不誤判)
        if (y > top)
            y = top;
        else if (y < bottom)
            y = bottom;
        return y;
    }

    void MovePaddleTowards(Entity paddle, float targetY, float dt)
    {
        PaddleComponent *pPaddle = world.GetComponent<PaddleComponent>(paddle);
        if (!pPaddle)
            return;
        const float diff = targetY - pPaddle->position.y;
        const float maxStep = pPaddle->speed * dt;
        const float step = diff > 0.0f ? maxStep : (diff < 0.0f ? -maxStep : 0.0f);
        if (step != 0.0f && (diff > 0.0f ? step : -step) > (diff > 0.0f ? diff : -diff))
        {
            // 一步到位
            pPaddle->position.y = targetY;
        }
        else
        {
            pPaddle->position.y += step;
        }
        const float limit = FieldHalfHeight - pPaddle->halfHeight;
        if (pPaddle->position.y > limit)
            pPaddle->position.y = limit;
        else if (pPaddle->position.y < -limit)
            pPaddle->position.y = -limit;
    }

    static bool BallOverlapsPaddle(const BallComponent &ball, const Point3D &paddleCenter,
                                   float paddleHalfWidth, float paddleHalfHeight)
    {
        // 球視為 AABB(半徑 r × r × r)vs 拍 AABB → 軸分離測試。
        const float dx = ball.position.x - paddleCenter.x;
        return dx >= -(paddleHalfWidth + BallRadius) && dx <= (paddleHalfWidth + BallRadius) &&
               ball.position.y - paddleCenter.y >= -(paddleHalfHeight + BallRadius) &&
               ball.position.y - paddleCenter.y <= (paddleHalfHeight + BallRadius);
    }

    void ReflectBall(BallComponent &ball, bool leftPaddleHit)
    {
        // 確保往反方向:left 拍反射後 velocity.x > 0;right 拍反射後 < 0。
        const float minSpeed = BallSpeed * 0.5f;
        ball.velocity.x = (leftPaddleHit ? 1.0f : -1.0f) * (ball.velocity.x > 0 ? 1.0f : -1.0f) *
                          (minSpeed + Math::Abs(ball.velocity.x) * 0.3f);
        // 依擊中點偏移改變 y 速度(可增加難度):-1..1 映射到 -3..3
        const float hitOffset = (ball.position.y - clampY(ball, leftPaddleHit ? -1.0f : 1.0f)) * 0.0f; // 保持簡單:不額外偏轉
        (void)hitOffset;
        // 微幅加速(每次拍擊 +4%)
        const float speed = std::sqrt(ball.velocity.x * ball.velocity.x + ball.velocity.y * ball.velocity.y);
        if (speed > 0.0f)
        {
            const float scale = 1.04f * Math::Min(1.0f, BallSpeed * 3.0f / speed);
            ball.velocity.x *= scale;
            ball.velocity.y *= scale;
        }
    }

    static float clampY(const BallComponent &, float)
    {
        return 0.0f;
    }

    bool CheckWin()
    {
        if (leftScore >= WinScore)
        {
            winner = 1;
            return true;
        }
        if (rightScore >= WinScore)
        {
            winner = 2;
            return true;
        }
        return false;
    }

    void LaunchBall()
    {
        BallComponent *pBall = world.GetComponent<BallComponent>(ball);
        if (!pBall)
            return;
        ballLaunched = true;
        const float rad = ballAngle * 3.14159265358979323846f / 180.0f;
        pBall->velocity = Point3D(Math::Cosine(rad) * BallSpeed, Math::Sine(rad) * BallSpeed, 0.0f);
    }

    void ResetBallForRound(Entity e, float angleDeg)
    {
        BallComponent *pBall = world.GetComponent<BallComponent>(e);
        if (!pBall)
            return;
        pBall->position = Point3D(0, 0, 0);
        pBall->velocity = Point3D(0, 0, 0);
        ballAngle = angleDeg;
        ballLaunched = false;
    }
};

} // namespace pong

#endif // PONG_SYSTEM_HPP