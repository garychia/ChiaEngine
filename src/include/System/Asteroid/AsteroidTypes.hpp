#ifndef ASTEROID_TYPES_HPP
#define ASTEROID_TYPES_HPP

#include "Data/Padding.hpp"
#include "Geometry/3D/Point3D.hpp"
#include <cstdint>

// Asteroids 的 Sim 層 POD 元件。position 以「世界單位」表示:場地 X ∈ [-10, 10]、Y ∈ [-8, 8]。
// 所有元件必須 POD、CHIA_PADDING_FREE。見 PongTypes.hpp / BreakoutTypes.hpp 註解。
namespace asteroid
{

enum class AsteroidSize : uint8_t
{
    Large = 3,   // 大:分裂成 2 顆中型
    Medium = 2,  // 中:分裂成 2 顆小型
    Small = 1,   // 小:被擊中後消滅
};

// 飞船的物理状态。
struct ShipComponent
{
    Point3D position;   // 中心(世界單位)
    float angle = 0.0f; // 朝向(度,0 = 朝上,-Y 方向)
    float speed = 5.0f; // 移動速率(世界單位/秒)
    float rotateSpeed = 180.0f; // 旋轉速率(度/秒)
    uint32_t alive = 1;         // 是否存活

    ShipComponent() = default;
    ShipComponent(const Point3D &pos, float a = 0.0f, float spd = 5.0f,
                  float rot = 180.0f, uint32_t al = 1)
        : position(pos), angle(a), speed(spd), rotateSpeed(rot), alive(al)
    {
    }
};

CHIA_PADDING_FREE(ShipComponent, &ShipComponent::position,
                  &ShipComponent::angle, &ShipComponent::speed,
                  &ShipComponent::rotateSpeed, &ShipComponent::alive);

// 子弹：从飞船发射，沿朝向直线飞行，寿命届满自动消失。
struct BulletComponent
{
    Point3D position;   // 中心(世界單位)
    Point3D velocity;   // 速度(世界單位/秒)
    uint32_t ttl = 60;  // 剩余寿命(帧数)，每 tick -1，到 0 消失

    BulletComponent() = default;
    BulletComponent(const Point3D &pos, const Point3D &vel, uint32_t t = 60)
        : position(pos), velocity(vel), ttl(t)
    {
    }
};

CHIA_PADDING_FREE(BulletComponent, &BulletComponent::position,
                  &BulletComponent::velocity, &BulletComponent::ttl);

// 小行星：被子弹击中后按大小分裂，小型被击中后消灭。
struct AsteroidComponent
{
    Point3D position;   // 中心(世界單位)
    Point3D velocity;   // 速度(世界單位/秒)
    uint32_t size = 3;  // 大小(3=大,2=中,1=小)
    uint32_t alive = 1; // 是否存活

    AsteroidComponent() = default;
    AsteroidComponent(const Point3D &pos, const Point3D &vel,
                      uint32_t s = 3, uint32_t al = 1)
        : position(pos), velocity(vel), size(s), alive(al)
    {
    }
};

CHIA_PADDING_FREE(AsteroidComponent, &AsteroidComponent::position,
                  &AsteroidComponent::velocity,
                  &AsteroidComponent::size, &AsteroidComponent::alive);

// 空标记型别：标识飞船/子弹/小行星实体(供 View 查询渲染)。
struct ShipTag
{
};
static_assert(::NoPadding<ShipTag>(), "ShipTag: empty type, no padding.");

struct BulletTag
{
};
static_assert(::NoPadding<BulletTag>(), "BulletTag: empty type, no padding.");

struct AsteroidTag
{
};
static_assert(::NoPadding<AsteroidTag>(), "AsteroidTag: empty type, no padding.");

} // namespace asteroid

#endif // ASTEROID_TYPES_HPP
