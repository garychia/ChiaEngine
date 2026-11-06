#ifndef BREAKOUT_TYPES_HPP
#define BREAKOUT_TYPES_HPP

#include "Data/Padding.hpp"
#include "Geometry/3D/Point3D.hpp"
#include <cstdint>

// Breakout 的 Sim 層 POD 元件。position 以「世界單位」表示:場地 X ∈ [-10, 10]、Y ∈ [-8, 8]。
// 所有元件必須 POD、CHIA_PADDING_FREE(World::Hash() 逐位元組讀)。見 PongTypes.hpp 註解。
namespace breakout
{

// 球的物理狀態。
struct BallComponent
{
    Point3D position;   // 球心(世界單位)
    Point3D velocity;   // 速度(世界單位/秒)
    float radius = 0.25f;   // 半徑

    BallComponent() = default;
    BallComponent(const Point3D &pos, const Point3D &vel, float r = 0.25f)
        : position(pos), velocity(vel), radius(r)
    {
    }
};

CHIA_PADDING_FREE(BallComponent, &BallComponent::position, &BallComponent::velocity,
                  &BallComponent::radius);

// 挡板(玩家操控，左右移动)。
struct PaddleComponent
{
    Point3D position;   // 中心
    float halfWidth = 1.2f;   // 半寬
    float halfHeight = 0.2f;  // 半高
    float speed = 10.0f;        // 移動速率(世界單位/秒)

    PaddleComponent() = default;
    PaddleComponent(const Point3D &pos, float hw = 1.2f, float hh = 0.2f,
                    float spd = 10.0f)
        : position(pos), halfWidth(hw), halfHeight(hh), speed(spd)
    {
    }
};

CHIA_PADDING_FREE(PaddleComponent, &PaddleComponent::position,
                  &PaddleComponent::halfWidth, &PaddleComponent::halfHeight,
                  &PaddleComponent::speed);

// 砖块(可被球击碎，击中后标记 destroyed，View 不再渲染)。
struct BrickComponent
{
    Point3D position;   // 中心
    float halfWidth = 0.9f;   // 半寬
    float halfHeight = 0.4f;  // 半高
    uint32_t score = 10;    // 击毁得分
    uint32_t alive = 1;     // 是否仍在场上(被击中后设 0)

    BrickComponent() = default;
    BrickComponent(const Point3D &pos, float hw = 0.9f, float hh = 0.4f,
                   uint32_t s = 10, uint32_t a = 1)
        : position(pos), halfWidth(hw), halfHeight(hh), score(s), alive(a)
    {
    }
};

CHIA_PADDING_FREE(BrickComponent, &BrickComponent::position,
                  &BrickComponent::halfWidth, &BrickComponent::halfHeight,
                  &BrickComponent::score, &BrickComponent::alive);

// 空标记型别：标识球/挡板/砖块实体(供 View 查询渲染)。
struct BallTag
{
};
static_assert(::NoPadding<BallTag>(), "BallTag: empty type, no padding.");

struct PaddleTag
{
};
static_assert(::NoPadding<PaddleTag>(), "PaddleTag: empty type, no padding.");

struct BrickTag
{
};
static_assert(::NoPadding<BrickTag>(), "BrickTag: empty type, no padding.");

} // namespace breakout

#endif // BREAKOUT_TYPES_HPP
