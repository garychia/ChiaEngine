#ifndef BREAKOUT_SYSTEM_HPP
#define BREAKOUT_SYSTEM_HPP

 // BreakoutSystem — Breakout/Arkanoid 的 Sim 層模組(確定性、無 GPU)。
 //
 // 設計(遵循 PongSystem 模式):
 //  - 自己擁有 World;模組對外只暴露「spawn/設定」接縫(#81 的模組隔離規則)
 //  - FixedUpdate 依固定 60Hz 步進:先讀 SimInput(context 服務),移動挡板,
 //    推進球(速度 × dt),處理牆壁/挡板/砖塊碰撞與得分。
 //  - 碰撟用軸分離 AABB-球判定(球視為 AABB 半徑 r,砖塊/挡板視為 AABB
 //    halfExtents)→ 完全確定性、無浮點分歧。
 //  - 砖塊被击中后標記 destroyed(不立即銷毀 Entity,避免 World 遍歷順序問題),
 //    View 不再渲染 destroyed 的砖塊。
 //
 // 控制:BitLeft/BitRight(actionBits bit0/bit1)= 挡板左/右移動。
 //       BitLaunch(bit2)= 發球/重啟。
 // 勝利:所有砖塊消滅;失敗:生命歸零。

#include "Math/Math.hpp"
#include "System/Module/EngineContext.hpp"
#include "System/Module/IModule.hpp"
#include "System/Module/SimInput.hpp"
#include "System/World/Entity.hpp"
#include "System/World/World.hpp"
#include "BreakoutTypes.hpp"

#include <cstdint>
#include <cmath>

namespace breakout
{

class BreakoutSystem : public IModule
{
  public:
    // 輸入位元(與 SimInput.actionBits 對應)
    static constexpr uint32_t BitLeft = 0x1u;   // 挡板向左
    static constexpr uint32_t BitRight = 0x2u;  // 挡板向右
    static constexpr uint32_t BitLaunch = 0x4u; // 發球/重啟

    // 場地(世界單位)
    static constexpr float FieldHalfWidth = 10.0f;
    static constexpr float FieldHalfHeight = 8.0f;

    // 挡板
    static constexpr float PaddleY = -7.0f;   // 挡板固定 Y
    static constexpr float PaddleMinX = -FieldHalfWidth + 1.0f;
    static constexpr float PaddleMaxX = FieldHalfWidth - 1.0f;

    // 球
    static constexpr float BallSpeed = 5.0f;

    // 生命與勝利
    static constexpr uint32_t MaxLives = 3;
    static constexpr uint32_t WinScore = 0; // 所有砖塊消滅即勝(用 bricksRemaining == 0 判斷)

    BreakoutSystem()
        : world(),
          pContext(nullptr),
          paddleEntity(Entity()),
          ballEntity(Entity()),
          lives(MaxLives),
          score(0),
          bricksRemaining(0),
          winner(0),
          gameOver(false),
          ballLaunched(false),
          roundTicks(0)
    {
    }

    // ---- 接縫(供 View 初始化場景;不暴露 World)----
    Entity CreatePaddle()
    {
        Entity e = world.CreateEntity();
        world.AddComponent<PaddleComponent>(e, PaddleComponent{Point3D(0, PaddleY, 0)});
        world.AddComponent<PaddleTag>(e, PaddleTag{});
        paddleEntity = e;
        return e;
    }

    Entity CreateBall(float angleDeg = 60.0f)
    {
        Entity e = world.CreateEntity();
        world.AddComponent<BallComponent>(
            e, BallComponent{Point3D(0, PaddleY + 0.6f, 0), Point3D(0, 0, 0)});
        world.AddComponent<BallTag>(e, BallTag{});
        ballEntity = e;
        ballAngle = angleDeg;
        ResetBall(e, angleDeg);
        return e;
    }

    // 生成砖塊網格:rows 行,cols 列,從 (startX, startY) 開始,每塊間距 spacing。
    // 回傳生成的砖塊 entity 數量。
    uint32_t BuildBrickGrid(uint32_t rows, uint32_t cols,
                            float startX = -8.5f, float startY = 4.0f,
                            float spacingX = 1.9f, float spacingY = 1.0f)
    {
        uint32_t count = 0;
        for (uint32_t r = 0; r < rows; r++)
        {
            for (uint32_t c = 0; c < cols; c++)
            {
                Entity e = world.CreateEntity();
                float x = startX + c * spacingX;
                float y = startY + r * spacingY;
                // 得分隨行數遞減(上方得分高)
                uint32_t brickScore = (rows - r) * 10;
                world.AddComponent<BrickComponent>(
                    e, BrickComponent{Point3D(x, y, 0), 0.9f, 0.4f, brickScore, 1});
                world.AddComponent<BrickTag>(e, BrickTag{});
                count++;
                bricksRemaining++;
            }
        }
        return count;
    }

    // ---- 查詢(View 讀整局狀態;Sim 是狀態源)----
    uint32_t GetLives() const { return lives; }
    uint32_t GetScore() const { return score; }
    uint32_t GetBricksRemaining() const { return bricksRemaining; }
    bool HasWinner() const { return winner != 0; }
    uint32_t GetWinner() const { return winner; } // 1 = 勝,2 = 遊戲結束
    bool IsGameOver() const { return gameOver; }

    // 測試需要用 World 直接改狀態。
    World &GetWorld() { return world; }

    // 重設整局(分數歸零、生命歸零、球回到原位、勝利清空)。
    void Reset()
    {
        lives = MaxLives;
        score = 0;
        bricksRemaining = 0;
        winner = 0;
        gameOver = false;
        ballLaunched = false;
        roundTicks = 0;

        // 銷毀所有砖塊(先收集 entity,避免遍歷 View 時修改池導致迭代器失效)
        DynamicArray<Entity> bricksToDestroy;
        {
            auto brickView = world.View<BrickComponent, BrickTag>();
            for (auto [e, brick, tag] : brickView)
            {
                (void)tag;
                (void)brick;
                bricksToDestroy.Append(e);
            }
        }
        for (size_t i = 0; i < bricksToDestroy.GetNElements(); i++)
            world.DestroyEntity(bricksToDestroy[i]);

        if (world.Alive(paddleEntity))
        {
            PaddleComponent *pPaddle = world.GetComponent<PaddleComponent>(paddleEntity);
            if (pPaddle)
                pPaddle->position.x = 0;
        }
        if (world.Alive(ballEntity))
        {
            ResetBall(ballEntity, ballAngle);
        }
    }

    // 測試用:強制發球(忽略 ballLaunched 狀態)
    void StartBallForTest()
    {
        if (world.Alive(ballEntity))
            LaunchBall(ballEntity);
    }

    uint64_t GetHash() const { return world.Hash(); }

    // ---- IModule ----
    void OnAttach(EngineContext &context) override
    {
        pContext = &context;
        context.RegisterService<BreakoutSystem>(this);
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

        // 遊戲結束後輸入不影響(除非 Reset)
        if (gameOver)
            return;

        // 勝利後輸入不影響
        if (bricksRemaining == 0 && winner == 0)
        {
            winner = 1;
            gameOver = true;
            return;
        }

        // 挡板移動(僅當 paddleEntity 已透過 CreatePaddle() 建立時有效)
        if (world.Alive(paddleEntity) && world.HasComponent<PaddleComponent>(paddleEntity))
        {
            PaddleComponent *pPaddle = world.GetComponent<PaddleComponent>(paddleEntity);
            if (pPaddle && pInput)
            {
                if (pInput->actionBits & BitLeft)
                    pPaddle->position.x -= pPaddle->speed * dt;
                if (pInput->actionBits & BitRight)
                    pPaddle->position.x += pPaddle->speed * dt;
                // clamp 到場地內
                if (pPaddle->position.x < PaddleMinX)
                    pPaddle->position.x = PaddleMinX;
                else if (pPaddle->position.x > PaddleMaxX)
                    pPaddle->position.x = PaddleMaxX;
            }
        }

        // 發球:未發射時由輸入觸發
        if (!ballLaunched && pInput &&
            (pInput->actionBits & BitLaunch))
        {
            LaunchBall(ballEntity);
        }

        // 球運動
        if (world.Alive(ballEntity) && world.HasComponent<BallComponent>(ballEntity))
        {
            BallComponent *pBall = world.GetComponent<BallComponent>(ballEntity);
            if (pBall && ballLaunched)
            {
                pBall->position.x += pBall->velocity.x * dt;
                pBall->position.y += pBall->velocity.y * dt;

                // 上下牆壁反射(Y = ±FieldHalfHeight)
                if (pBall->position.y > FieldHalfHeight - pBall->radius)
                {
                    pBall->position.y = FieldHalfHeight - pBall->radius;
                    pBall->velocity.y = -pBall->velocity.y;
                }
                else if (pBall->position.y < -FieldHalfHeight + pBall->radius)
                {
                    // 球掉出底部:扣生命,重置球
                    lives--;
                    if (lives == 0)
                    {
                        winner = 2;
                        gameOver = true;
                    }
                    else
                    {
                        ResetBall(ballEntity, ballAngle);
                    }
                    return; // 本幀不再處理碰播
                }

                // 左右牆壁反射(X = ±FieldHalfWidth)
                if (pBall->position.x > FieldHalfWidth - pBall->radius)
                {
                    pBall->position.x = FieldHalfWidth - pBall->radius;
                    pBall->velocity.x = -pBall->velocity.x;
                }
                else if (pBall->position.x < -FieldHalfWidth + pBall->radius)
                {
                    pBall->position.x = -FieldHalfWidth + pBall->radius;
                    pBall->velocity.x = -pBall->velocity.x;
                }

                // 球与挡板碰撞
                if (world.Alive(paddleEntity))
                {
                    PaddleComponent *pPaddle =
                        world.GetComponent<PaddleComponent>(paddleEntity);
                    if (pPaddle && BallOverlapsPaddle(*pBall, *pPaddle))
                    {
                        ReflectBallOffPaddle(*pBall, *pPaddle);
                    }
                }

                // 球与砖塊碰撞(每幀最多处理一块,避免多重碰播导致的跳过)
                HandleBrickCollisions(*pBall);
            }
        }
    }

    void Update(const FrameClock &clock) override
    {
        (void)clock;
    }

  private:
    World world;
    EngineContext *pContext;

    Entity paddleEntity;
    Entity ballEntity;

    uint32_t lives;
    uint32_t score;
    uint32_t bricksRemaining;
    uint32_t winner;
    bool gameOver;
    bool ballLaunched;
    float ballAngle;
    uint32_t roundTicks;

    void ResetBall(Entity e, float angleDeg)
    {
        BallComponent *pBall = world.GetComponent<BallComponent>(e);
        if (!pBall)
            return;
        pBall->position = Point3D(0, PaddleY + 0.6f, 0);
        pBall->velocity = Point3D(0, 0, 0);
        ballAngle = angleDeg;
        ballLaunched = false;
    }

    void LaunchBall(Entity e)
    {
        BallComponent *pBall = world.GetComponent<BallComponent>(e);
        if (!pBall || ballLaunched)
            return;
        ballLaunched = true;
        const float rad = ballAngle * 3.14159265358979323846f / 180.0f;
        // 发球方向向上(正 Y),X 分量随角度
        pBall->velocity = Point3D(Math::Cosine(rad) * BallSpeed,
                                  Math::Sine(rad) * BallSpeed, 0.0f);
    }

    // 球(AABB 半徑 r)与挡板(AABB halfExtents)重叠判定。
    static bool BallOverlapsPaddle(const BallComponent &ball,
                                   const PaddleComponent &paddle)
    {
        const float dx = ball.position.x - paddle.position.x;
        const float dy = ball.position.y - paddle.position.y;
        return dx >= -(paddle.halfWidth + ball.radius) &&
               dx <= (paddle.halfWidth + ball.radius) &&
               dy >= -(paddle.halfHeight + ball.radius) &&
               dy <= (paddle.halfHeight + ball.radius);
    }

    // 球击中挡板后的反射:只反转 Y 方向,根据击中位置在挡板上的相对位置调整 X 速度。
    void ReflectBallOffPaddle(BallComponent &ball,
                              const PaddleComponent &paddle)
    {
        ball.velocity.y = Math::Abs(ball.velocity.y); // 向上弹
        // 击中位置偏移(-1..1):左侧负,右侧正
        float hitOffset = (ball.position.x - paddle.position.x) /
                          (paddle.halfWidth + ball.radius);
        if (hitOffset > 1.0f)
            hitOffset = 1.0f;
        else if (hitOffset < -1.0f)
            hitOffset = -1.0f;
        ball.velocity.x = hitOffset * BallSpeed * 1.2f;
        // 微幅加速(每次拍击 +3%,速度上限 3×BallSpeed)
        float speed = std::sqrt(ball.velocity.x * ball.velocity.x +
                                ball.velocity.y * ball.velocity.y);
        if (speed > 0.0f)
        {
            float scale = 1.03f * Math::Min(1.0f, BallSpeed * 3.0f / speed);
            ball.velocity.x *= scale;
            ball.velocity.y *= scale;
        }
    }

    // 处理球与砖塊碰撞(每帧最多处理一块)。击中后砖塊标记 destroyed,得分,球反射。
    void HandleBrickCollisions(BallComponent &ball)
    {
        auto brickView = world.View<BrickComponent, BrickTag>();
        for (auto [e, brick, tag] : brickView)
        {
            (void)tag;
            if (!brick->alive)
                continue;
            if (BallOverlapsBrick(ball, *brick))
            {
                // 标记砖塊 destroyed
                brick->alive = false;
                bricksRemaining--;
                score += brick->score;

                // 根据击中边决定反射方向(简单:反转 Y)
                ball.velocity.y = -ball.velocity.y;

                // 只处理第一块碰撞的砖塊
                break;
            }
        }
    }

    // 球(AABB 半徑 r)与砖塊(AABB halfExtents)重叠判定。
    static bool BallOverlapsBrick(const BallComponent &ball,
                                  const BrickComponent &brick)
    {
        const float dx = ball.position.x - brick.position.x;
        const float dy = ball.position.y - brick.position.y;
        return dx >= -(brick.halfWidth + ball.radius) &&
               dx <= (brick.halfWidth + ball.radius) &&
               dy >= -(brick.halfHeight + ball.radius) &&
               dy <= (brick.halfHeight + ball.radius);
    }
};

} // namespace breakout

#endif // BREAKOUT_SYSTEM_HPP
