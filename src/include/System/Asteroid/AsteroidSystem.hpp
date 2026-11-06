#ifndef ASTEROID_SYSTEM_HPP
#define ASTEROID_SYSTEM_HPP

 // AsteroidSystem — Asteroids 的 Sim 層模組(確定性、無 GPU)。
 //
 // 設計(遵循 Pong/Breakout 模式):
 //  - 自己擁有 World;模組對外只暴露「spawn/設定」接縫
 //  - FixedUpdate 依固定 60Hz 步進:讀 SimInput,更新飞船旋转/加速/射击,
 //    推進子弹和小行星,處理碰撞(子弹-小行星分裂,飞船-小行星扣生命)。
 //  - 屏幕包围:超出場地邊界從另一邊出現。
 //  - 所有小行星消滅後进入下一波(小行星数量+2)。
 //
 // 控制:BitLeft/BitRight=逆时针/顺时针旋转,BitUp=加速,BitLaunch=射击。
 // 勝利:无限波次,直到生命歸零。

#include "Math/Math.hpp"
#include "System/Module/EngineContext.hpp"
#include "System/Module/IModule.hpp"
#include "System/Module/SimInput.hpp"
#include "System/World/Entity.hpp"
#include "System/World/World.hpp"
#include "AsteroidTypes.hpp"

#include <cstdint>
#include <cmath>

namespace asteroid
{

class AsteroidSystem : public IModule
{
  public:
    // 輸入位元
    static constexpr uint32_t BitLeft = 0x1u;   // 逆時針旋轉
    static constexpr uint32_t BitRight = 0x2u;  // 順時針旋轉
    static constexpr uint32_t BitUp = 0x4u;     // 加速(朝朝向方向)
    static constexpr uint32_t BitLaunch = 0x8u; // 射擊

    // 場地(世界單位)
    static constexpr float FieldHalfWidth = 10.0f;
    static constexpr float FieldHalfHeight = 8.0f;

    // 飞船
    static constexpr float ShipStartX = 0.0f;
    static constexpr float ShipStartY = 0.0f;
    static constexpr float MaxShipSpeed = 6.0f;
    static constexpr float ShipAccel = 4.0f;
    static constexpr float ShipFriction = 0.98f; // 每 tick 速度衰減

    // 子弹
    static constexpr float BulletSpeed = 7.0f;
    static constexpr uint32_t BulletTTL = 60; // 帧

    // 小行星大小常量
    static constexpr uint32_t AsteroidSizeLarge = 3;
    static constexpr uint32_t AsteroidSizeMedium = 2;
    static constexpr uint32_t AsteroidSizeSmall = 1;

    // 小行星
    static constexpr uint32_t InitialAsteroidCount = 4;
    static constexpr float AsteroidSpeedMin = 1.0f;
    static constexpr float AsteroidSpeedMax = 2.5f;

    // 得分
    static constexpr uint32_t ScoreLarge = 20;
    static constexpr uint32_t ScoreMedium = 50;
    static constexpr uint32_t ScoreSmall = 100;

    // 生命
    static constexpr uint32_t MaxLives = 3;

    AsteroidSystem()
        : world(),
          pContext(nullptr),
          shipEntity(Entity()),
          lives(MaxLives),
          score(0),
          wave(1),
          asteroidsRemaining(0),
          winner(0),
          gameOver(false)
    {
    }

    // ---- 接縫 ----
    Entity CreateShip()
    {
        Entity e = world.CreateEntity();
        world.AddComponent<ShipComponent>(
            e, ShipComponent{Point3D(ShipStartX, ShipStartY, 0)});
        world.AddComponent<ShipTag>(e, ShipTag{});
        shipEntity = e;
        return e;
    }

    Entity CreateBullet(const Point3D &pos, const Point3D &vel)
    {
        Entity e = world.CreateEntity();
        world.AddComponent<BulletComponent>(e, BulletComponent{pos, vel, BulletTTL});
        world.AddComponent<BulletTag>(e, BulletTag{});
        return e;
    }

    Entity CreateAsteroid(const Point3D &pos, const Point3D &vel,
                          uint32_t size)
    {
        Entity e = world.CreateEntity();
        world.AddComponent<AsteroidComponent>(
            e, AsteroidComponent{pos, vel, size, 1});
        world.AddComponent<AsteroidTag>(e, AsteroidTag{});
        asteroidsRemaining++;
        return e;
    }

    // 生成初始小行星波次:count 顆大型小行星,位置固定(確定性)。
    void SpawnWave(uint32_t count)
    {
        float startAngle = 0.0f;
        for (uint32_t i = 0; i < count; i++)
        {
            float angle = startAngle + i * (6.283185307f / count);
            float dist = 6.0f;
            Point3D pos(std::cos(angle) * dist, std::sin(angle) * dist, 0.f);
            float velAngle = angle + 3.1415926535f / 2.0f;
            float speed = AsteroidSpeedMin +
                          (static_cast<float>(i % 3) + 1.f) *
                              (AsteroidSpeedMax - AsteroidSpeedMin) / 3.f;
            Point3D vel(std::cos(velAngle) * speed,
                        std::sin(velAngle) * speed, 0.f);
            CreateAsteroid(pos, vel, AsteroidSizeLarge);
        }
    }

    // ---- 查詢 ----
    uint32_t GetLives() const { return lives; }
    uint32_t GetScore() const { return score; }
    uint32_t GetWave() const { return wave; }
    uint32_t GetAsteroidsRemaining() const { return asteroidsRemaining; }
    bool IsGameOver() const { return gameOver; }
    World &GetWorld() { return world; }

    void Reset()
    {
        lives = MaxLives;
        score = 0;
        wave = 1;
        asteroidsRemaining = 0;
        gameOver = false;
        winner = 0;

        // 销毁所有实体(先收集,避免遍历 View 时修改池)
        DynamicArray<Entity> toDestroy;
        {
            auto allView = world.View<ShipTag, BulletTag, AsteroidTag>();
            for (auto [e, sTag, bTag, aTag] : allView)
            {
                (void)sTag;
                (void)bTag;
                (void)aTag;
                toDestroy.Append(e);
            }
        }
        for (size_t i = 0; i < toDestroy.GetNElements(); i++)
            world.DestroyEntity(toDestroy[i]);

        CreateShip();
        SpawnWave(InitialAsteroidCount);
    }

    uint64_t GetHash() const { return world.Hash(); }

    // ---- IModule ----
    void OnAttach(EngineContext &context) override
    {
        pContext = &context;
        context.RegisterService<AsteroidSystem>(this);
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

        // 飞船控制
        if (world.Alive(shipEntity) &&
            world.HasComponent<ShipComponent>(shipEntity))
        {
            ShipComponent *pShip = world.GetComponent<ShipComponent>(shipEntity);
            if (pShip && pInput)
            {
                // 旋转
                if (pInput->actionBits & BitLeft)
                    pShip->angle += pShip->rotateSpeed * dt;
                if (pInput->actionBits & BitRight)
                    pShip->angle -= pShip->rotateSpeed * dt;
                while (pShip->angle >= 360.0f)
                    pShip->angle -= 360.0f;
                while (pShip->angle < 0.0f)
                    pShip->angle += 360.0f;
            }
        }

        // 射击
        if (pInput && (pInput->actionBits & BitLaunch))
        {
            if (world.Alive(shipEntity) &&
                world.HasComponent<ShipComponent>(shipEntity))
            {
                ShipComponent *pShip =
                    world.GetComponent<ShipComponent>(shipEntity);
                if (pShip)
                {
                    float rad = pShip->angle * 3.14159265358979323846f /
                                180.0f;
                    Point3D dir(std::sin(rad), -std::cos(rad), 0.f);
                    Point3D bulletPos =
                        pShip->position + dir * 1.0f;
                    Point3D bulletVel = dir * BulletSpeed;
                    CreateBullet(bulletPos, bulletVel);
                }
            }
        }

        // 推进子弹
        auto bulletView = world.View<BulletComponent, BulletTag>();
        DynamicArray<Entity> bulletsToDestroy;
        for (auto [e, bullet, bTag] : bulletView)
        {
            (void)bTag;
            bullet->position.x += bullet->velocity.x * dt;
            bullet->position.y += bullet->velocity.y * dt;
            bullet->ttl--;
            if (bullet->ttl == 0)
                bulletsToDestroy.Append(e);
        }
        for (size_t i = 0; i < bulletsToDestroy.GetNElements(); i++)
            world.DestroyEntity(bulletsToDestroy[i]);

        // 推进小行星 + 屏幕包围
        auto asteroidView = world.View<AsteroidComponent, AsteroidTag>();
        for (auto [e, ast, aTag] : asteroidView)
        {
            (void)aTag;
            if (!ast->alive)
                continue;
            ast->position.x += ast->velocity.x * dt;
            ast->position.y += ast->velocity.y * dt;
            if (ast->position.x > FieldHalfWidth)
                ast->position.x = -FieldHalfWidth;
            else if (ast->position.x < -FieldHalfWidth)
                ast->position.x = FieldHalfWidth;
            if (ast->position.y > FieldHalfHeight)
                ast->position.y = -FieldHalfHeight;
            else if (ast->position.y < -FieldHalfHeight)
                ast->position.y = FieldHalfHeight;
        }

        // 子弹-小行星碰撞
        HandleBulletAsteroidCollisions();

        // 飞船-小行星碰撞
        HandleShipAsteroidCollisions();

        // 所有小行星消灭 → 下一波
        if (asteroidsRemaining == 0)
        {
            wave++;
            SpawnWave(InitialAsteroidCount + wave);
        }
    }

    void Update(const FrameClock &clock) override
    {
        (void)clock;
    }

  private:
    World world;
    EngineContext *pContext;

    Entity shipEntity;
    uint32_t lives;
    uint32_t score;
    uint32_t wave;
    uint32_t asteroidsRemaining;
    uint32_t winner;
    bool gameOver;

    void HandleBulletAsteroidCollisions()
    {
        auto bulletView = world.View<BulletComponent, BulletTag>();
        DynamicArray<Entity> bulletsToDestroy;
        for (auto [be, bullet, bTag] : bulletView)
        {
            (void)bTag;
            if (bullet->ttl == 0)
                continue;

            auto asteroidView = world.View<AsteroidComponent, AsteroidTag>();
            for (auto [ae, ast, aTag] : asteroidView)
            {
                (void)aTag;
                if (!ast->alive)
                    continue;
                if (BulletOverlapsAsteroid(*bullet, *ast))
                {
                    ast->alive = 0;
                    asteroidsRemaining--;
                    score += AsteroidScore(*ast);
                    SplitAsteroid(*ast);
                    bullet->ttl = 0;
                    bulletsToDestroy.Append(be);
                    break;
                }
            }
        }
        for (size_t i = 0; i < bulletsToDestroy.GetNElements(); i++)
            world.DestroyEntity(bulletsToDestroy[i]);
    }

    void HandleShipAsteroidCollisions()
    {
        if (!world.Alive(shipEntity) ||
            !world.HasComponent<ShipComponent>(shipEntity))
            return;
        ShipComponent *pShip = world.GetComponent<ShipComponent>(shipEntity);
        if (!pShip || !pShip->alive)
            return;

        auto asteroidView = world.View<AsteroidComponent, AsteroidTag>();
        for (auto [ae, ast, aTag] : asteroidView)
        {
            (void)aTag;
            if (!ast->alive)
                continue;
            if (ShipOverlapsAsteroid(*pShip, *ast))
            {
                lives--;
                if (lives == 0)
                {
                    gameOver = true;
                    winner = 2;
                }
                else
                {
                    pShip->position = Point3D(ShipStartX, ShipStartY, 0);
                    pShip->angle = 0.0f;
                }
                break;
            }
        }
    }

    void SplitAsteroid(const AsteroidComponent &ast)
    {
        if (ast.size == AsteroidSizeSmall)
            return;
        uint32_t childSize = (ast.size == AsteroidSizeLarge) ? AsteroidSizeMedium
                                                             : AsteroidSizeSmall;
        float baseAngle = 0.0f;
        for (int i = 0; i < 2; i++)
        {
            float angle = baseAngle + i * 3.1415926535f;
            float speed = AsteroidSpeedMin + 0.5f;
            Point3D vel(std::cos(angle) * speed, std::sin(angle) * speed, 0.f);
            CreateAsteroid(ast.position, vel, childSize);
        }
    }

    uint32_t AsteroidScore(const AsteroidComponent &ast) const
    {
        if (ast.size == AsteroidSizeLarge)
            return ScoreLarge;
        if (ast.size == AsteroidSizeMedium)
            return ScoreMedium;
        if (ast.size == AsteroidSizeSmall)
            return ScoreSmall;
        return 0;
    }

    static bool BulletOverlapsAsteroid(const BulletComponent &bullet,
                                       const AsteroidComponent &ast)
    {
        const float bulletRadius = 0.15f;
        float astRadius = 0.6f;
        if (ast.size == AsteroidSizeMedium)
            astRadius = 0.4f;
        else if (ast.size == AsteroidSizeSmall)
            astRadius = 0.25f;
        const float dx = bullet.position.x - ast.position.x;
        const float dy = bullet.position.y - ast.position.y;
        return dx >= -(astRadius + bulletRadius) &&
               dx <= (astRadius + bulletRadius) &&
               dy >= -(astRadius + bulletRadius) &&
               dy <= (astRadius + bulletRadius);
    }

    static bool ShipOverlapsAsteroid(const ShipComponent &ship,
                                     const AsteroidComponent &ast)
    {
        const float shipRadius = 0.5f;
        float astRadius = 0.6f;
        if (ast.size == AsteroidSizeMedium)
            astRadius = 0.4f;
        else if (ast.size == AsteroidSizeSmall)
            astRadius = 0.25f;
        const float dx = ship.position.x - ast.position.x;
        const float dy = ship.position.y - ast.position.y;
        return dx >= -(astRadius + shipRadius) &&
               dx <= (astRadius + shipRadius) &&
               dy >= -(astRadius + shipRadius) &&
               dy <= (astRadius + shipRadius);
    }
};

} // namespace asteroid

#endif // ASTEROID_SYSTEM_HPP
