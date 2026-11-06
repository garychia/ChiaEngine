#ifndef PLATFORMER_SYSTEM_HPP
#define PLATFORMER_SYSTEM_HPP

 // PlatformerSystem — 平台跳躍的 Sim 層模組(確定性、無 GPU)。
 //
 // 設計(遵循 Pong/Breakout/Asteroids 模式):
 //  - 自己擁有 World;模組對外只暴露「spawn/設定」接縫
 //  - FixedUpdate 依固定 60Hz 步進:讀 SimInput,套用重力、玩家移動、跳躍,
 //    AABB 碰撞與平台解決、地面檢測。
 //  - 碰撞解決沿最小重疊軸(MTV):水平碰撞滑行,垂直碰撞停止並設定 onGround。
 //
 // 控制:BitLeft/BitRight=左右移動,BitLaunch=跳躍(僅 onGround 時觸發)。
 // 勝利:無最終勝利,持續遊戲直到重設。

#include "Math/Math.hpp"
#include "System/Module/EngineContext.hpp"
#include "System/Module/IModule.hpp"
#include "System/Module/SimInput.hpp"
#include "System/World/Entity.hpp"
#include "System/World/World.hpp"
#include "PlatformerTypes.hpp"

#include <cstdint>
#include <cmath>

namespace platformer
{

class PlatformerSystem : public IModule
{
  public:
    // 輸入位元
    static constexpr uint32_t BitLeft = 0x1u;   // 向左移動
    static constexpr uint32_t BitRight = 0x2u;  // 向右移動
    static constexpr uint32_t BitLaunch = 0x4u; // 跳躍

    // 場地(世界單位)
    static constexpr float FieldHalfWidth = 12.0f;
    static constexpr float FieldHalfHeight = 6.0f;

    // 重力(世界單位/秒^2,向下為負 Y)
    static constexpr float Gravity = 24.0f;

    // 玩家初始位置
    static constexpr float PlayerStartX = 0.0f;
    static constexpr float PlayerStartY = 2.0f;

    // 終端下落速度上限
    static constexpr float MaxFallSpeed = 15.0f;

    PlatformerSystem()
        : world(),
          pContext(nullptr),
          playerEntity(Entity()),
          lives(3),
          score(0),
          gameOver(false)
    {
    }

    // ---- 接縫 ----
    Entity CreatePlayer()
    {
        Entity e = world.CreateEntity();
        world.AddComponent<PlayerComponent>(
            e, PlayerComponent{Point3D(PlayerStartX, PlayerStartY, 0)});
        world.AddComponent<PlayerTag>(e, PlayerTag{});
        playerEntity = e;
        return e;
    }

    Entity CreatePlatform(const Point3D &pos, float width = 2.0f,
                          float height = 0.4f)
    {
        Entity e = world.CreateEntity();
        world.AddComponent<PlatformComponent>(
            e, PlatformComponent{pos, width, height});
        world.AddComponent<PlatformTag>(e, PlatformTag{});
        return e;
    }

    // 建立預設關卡:幾個平台(確定性位置)。
    void BuildDefaultLevel()
    {
        // 地板
        CreatePlatform(Point3D(0, -4.5f, 0), 10.0f, 0.4f);
        // 中間平台
        CreatePlatform(Point3D(-6, -1.5f, 0), 4.0f, 0.4f);
        CreatePlatform(Point3D(2, 0.5f, 0), 4.0f, 0.4f);
        CreatePlatform(Point3D(-2, 2.5f, 0), 4.0f, 0.4f);
        // 上方平台
        CreatePlatform(Point3D(6, 1.5f, 0), 3.0f, 0.4f);
    }

    // ---- 查詢 ----
    uint32_t GetLives() const { return lives; }
    uint32_t GetScore() const { return score; }
    bool IsGameOver() const { return gameOver; }
    bool IsOnGround() const
    {
        if (!world.Alive(playerEntity) ||
            !world.HasComponent<PlayerComponent>(playerEntity))
            return false;
        const PlayerComponent *p = world.GetComponent<PlayerComponent>(playerEntity);
        return p->onGround != 0;
    }
    World &GetWorld() { return world; }

    void Reset()
    {
        lives = 3;
        score = 0;
        gameOver = false;

        // 銷毀所有實體(先收集)
        DynamicArray<Entity> toDestroy;
        {
            auto allView = world.View<PlayerTag, PlatformTag>();
            for (auto [e, pTag, platTag] : allView)
            {
                (void)pTag;
                (void)platTag;
                toDestroy.Append(e);
            }
        }
        for (size_t i = 0; i < toDestroy.GetNElements(); i++)
            world.DestroyEntity(toDestroy[i]);

        CreatePlayer();
        BuildDefaultLevel();
    }

    uint64_t GetHash() const { return world.Hash(); }

    // ---- IModule ----
    void OnAttach(EngineContext &context) override
    {
        pContext = &context;
        context.RegisterService<PlatformerSystem>(this);
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

        if (gameOver)
            return;

        // ---- 1. 玩家輸入:左右移動 ----
        if (world.Alive(playerEntity) &&
            world.HasComponent<PlayerComponent>(playerEntity))
        {
            PlayerComponent *pPlayer =
                world.GetComponent<PlayerComponent>(playerEntity);
            if (pPlayer && pInput)
            {
                if (pInput->actionBits & BitLeft)
                    pPlayer->velocity.x = -pPlayer->moveSpeed;
                else if (pInput->actionBits & BitRight)
                    pPlayer->velocity.x = pPlayer->moveSpeed;
                else
                    pPlayer->velocity.x = 0.0f;

                // 跳躍:僅在地面且可跳躍時觸發
                if ((pInput->actionBits & BitLaunch) && pPlayer->onGround &&
                    pPlayer->canJump)
                {
                    pPlayer->velocity.y = pPlayer->jumpSpeed;
                    pPlayer->onGround = 0;
                    pPlayer->canJump = 0;
                }
                // 松開跳躍鍵後重設 canJump
                if (!(pInput->actionBits & BitLaunch))
                    pPlayer->canJump = 1;
            }
        }

        // ---- 2. 套用重力 ----
        if (world.Alive(playerEntity) &&
            world.HasComponent<PlayerComponent>(playerEntity))
        {
            PlayerComponent *pPlayer =
                world.GetComponent<PlayerComponent>(playerEntity);
            if (pPlayer)
            {
                pPlayer->velocity.y -= Gravity * dt;
                if (pPlayer->velocity.y < -MaxFallSpeed)
                    pPlayer->velocity.y = -MaxFallSpeed;
            }
        }

        // ---- 3. 應用速度到位置(X 先) ----
        ApplyMovementX(dt);
        ResolveCollisionsX();

        ApplyMovementY(dt);
        ResolveCollisionsY();
    }

    void Update(const FrameClock &clock) override
    {
        (void)clock;
    }

  private:
    World world;
    EngineContext *pContext;

    Entity playerEntity;
    uint32_t lives;
    uint32_t score;
    bool gameOver;

    // 沿 X 軸應用速度。
    void ApplyMovementX(float dt)
    {
        if (!world.Alive(playerEntity) ||
            !world.HasComponent<PlayerComponent>(playerEntity))
            return;
        PlayerComponent *pPlayer =
            world.GetComponent<PlayerComponent>(playerEntity);
        if (!pPlayer)
            return;
        pPlayer->position.x += pPlayer->velocity.x * dt;
    }

    // 解決 X 軸平台碰撞(水平滑行)。
    void ResolveCollisionsX()
    {
        if (!world.Alive(playerEntity) ||
            !world.HasComponent<PlayerComponent>(playerEntity))
            return;
        PlayerComponent *pPlayer =
            world.GetComponent<PlayerComponent>(playerEntity);
        if (!pPlayer)
            return;

        auto platformView = world.View<PlatformComponent, PlatformTag>();
        for (auto [pe, plat, pTag] : platformView)
        {
            (void)pTag;
            if (AABBOverlap(*pPlayer, *plat))
            {
                // 計算水平重疊深度
                float overlapLeft = (pPlayer->position.x + pPlayer->halfWidth) -
                                    (plat->position.x - plat->width / 2.f);
                float overlapRight = (plat->position.x + plat->width / 2.f) -
                                     (pPlayer->position.x - pPlayer->halfWidth);
                float minOverlap =
                    (overlapLeft < overlapRight) ? overlapLeft : overlapRight;

                if (pPlayer->velocity.x > 0.0f)
                {
                    // 向右移動,推回平台左邊
                    pPlayer->position.x = plat->position.x - plat->width / 2.f -
                                          pPlayer->halfWidth - 0.001f;
                }
                else if (pPlayer->velocity.x < 0.0f)
                {
                    // 向左移動,推回平台右邊
                    pPlayer->position.x = plat->position.x + plat->width / 2.f +
                                          pPlayer->halfWidth + 0.001f;
                }
                pPlayer->velocity.x = 0.0f;
            }
        }
    }

    // 沿 Y 軸應用速度。
    void ApplyMovementY(float dt)
    {
        if (!world.Alive(playerEntity) ||
            !world.HasComponent<PlayerComponent>(playerEntity))
            return;
        PlayerComponent *pPlayer =
            world.GetComponent<PlayerComponent>(playerEntity);
        if (!pPlayer)
            return;
        pPlayer->position.y += pPlayer->velocity.y * dt;
    }

    // 解決 Y 軸平台碰撞(垂直停止,設定 onGround)。
    void ResolveCollisionsY()
    {
        if (!world.Alive(playerEntity) ||
            !world.HasComponent<PlayerComponent>(playerEntity))
            return;
        PlayerComponent *pPlayer =
            world.GetComponent<PlayerComponent>(playerEntity);
        if (!pPlayer)
            return;

        pPlayer->onGround = 0; // 預設不在地面,碰撞後可能設為 1

        auto platformView = world.View<PlatformComponent, PlatformTag>();
        for (auto [pe, plat, pTag] : platformView)
        {
            (void)pTag;
            if (!AABBOverlap(*pPlayer, *plat))
                continue;

            // 計算垂直重疊深度
            float overlapTop = (pPlayer->position.y + pPlayer->halfHeight) -
                               (plat->position.y - plat->height / 2.f);
            float overlapBottom = (plat->position.y + plat->height / 2.f) -
                                  (pPlayer->position.y - pPlayer->halfHeight);
            float minOverlap =
                (overlapTop < overlapBottom) ? overlapTop : overlapBottom;

            if (pPlayer->velocity.y < 0.0f && overlapTop > 0.0f)
            {
                // 向下落,腳部碰到平台頂部 → 落在地面
                pPlayer->position.y =
                    plat->position.y + plat->height / 2.f + pPlayer->halfHeight +
                    0.001f;
                pPlayer->velocity.y = 0.0f;
                pPlayer->onGround = 1;
            }
            else if (pPlayer->velocity.y > 0.0f && overlapBottom > 0.0f)
            {
                // 向上移,頭部碰到平台底部
                pPlayer->position.y =
                    plat->position.y - plat->height / 2.f - pPlayer->halfHeight -
                    0.001f;
                pPlayer->velocity.y = 0.0f;
            }
        }

        // 場地邊界:底部掉出 → 扣生命/重設
        if (pPlayer->position.y - pPlayer->halfHeight < -FieldHalfHeight - 2.0f)
        {
            lives--;
            if (lives == 0)
            {
                gameOver = true;
            }
            else
            {
                ResetPlayerPosition();
            }
        }
    }

    void ResetPlayerPosition()
    {
        if (!world.Alive(playerEntity) ||
            !world.HasComponent<PlayerComponent>(playerEntity))
            return;
        PlayerComponent *pPlayer =
            world.GetComponent<PlayerComponent>(playerEntity);
        if (pPlayer)
        {
            pPlayer->position = Point3D(PlayerStartX, PlayerStartY, 0);
            pPlayer->velocity = Point3D(0, 0, 0);
            pPlayer->onGround = 0;
            pPlayer->canJump = 1;
        }
    }

    // AABB 重疊測試:玩家 vs 平台。
    static bool AABBOverlap(const PlayerComponent &p,
                            const PlatformComponent &plat)
    {
        return p.position.x + p.halfWidth > plat.position.x - plat.width / 2.f &&
               p.position.x - p.halfWidth < plat.position.x + plat.width / 2.f &&
               p.position.y + p.halfHeight > plat.position.y - plat.height / 2.f &&
               p.position.y - p.halfHeight < plat.position.y + plat.height / 2.f;
    }
};

} // namespace platformer

#endif // PLATFORMER_SYSTEM_HPP
