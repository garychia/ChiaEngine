#ifndef PLATFORMER_TYPES_HPP
#define PLATFORMER_TYPES_HPP

#include "Data/Padding.hpp"
#include "Geometry/3D/Point3D.hpp"
#include <cstdint>

// Platformer 的 Sim 層 POD 元件。position 以「世界單位」表示:場地 X ∈ [-12, 12]、Y ∈ [-6, 6]。
// 所有元件必須 POD、CHIA_PADDING_FREE。見 PongTypes.hpp / BreakoutTypes.hpp / AsteroidTypes.hpp 註解。
namespace platformer
{

// 玩家的物理狀態。
struct PlayerComponent
{
    Point3D position;   // 中心(世界單位)
    Point3D velocity;   // 速度(世界單位/秒)
    float halfWidth = 0.4f;    // 碰撞半寬
    float halfHeight = 0.6f;   // 碰撞半高
    float moveSpeed = 6.0f;    // 左右移動速率
    float jumpSpeed = 9.0f;    // 跳躍初始向上速度
    uint32_t onGround = 0;     // 是否在地面(腳部接觸平台頂部)
    uint32_t canJump = 1;      // 是否可跳躍(防止連續按住跳躍鍵觸發多次)

    PlayerComponent() = default;
    PlayerComponent(const Point3D &pos, const Point3D &vel = Point3D(0, 0, 0),
                    float hw = 0.4f, float hh = 0.6f,
                    float move = 6.0f, float jump = 9.0f,
                    uint32_t og = 0, uint32_t cj = 1)
        : position(pos), velocity(vel), halfWidth(hw), halfHeight(hh),
          moveSpeed(move), jumpSpeed(jump), onGround(og), canJump(cj)
    {
    }
};

CHIA_PADDING_FREE(PlayerComponent, &PlayerComponent::position,
                  &PlayerComponent::velocity, &PlayerComponent::halfWidth,
                  &PlayerComponent::halfHeight, &PlayerComponent::moveSpeed,
                  &PlayerComponent::jumpSpeed, &PlayerComponent::onGround,
                  &PlayerComponent::canJump);

// 靜態平台(不移動的碰撞體)。
struct PlatformComponent
{
    Point3D position;   // 中心(世界單位)
    float width = 2.0f;  // 寬度
    float height = 0.4f; // 高度

    PlatformComponent() = default;
    PlatformComponent(const Point3D &pos, float w = 2.0f, float h = 0.4f)
        : position(pos), width(w), height(h)
    {
    }
};

CHIA_PADDING_FREE(PlatformComponent, &PlatformComponent::position,
                  &PlatformComponent::width, &PlatformComponent::height);

// 空標記型別:標識玩家/平台實體(供 View 查詢渲染)。
struct PlayerTag
{
};
static_assert(::NoPadding<PlayerTag>(), "PlayerTag: empty type, no padding.");

struct PlatformTag
{
};
static_assert(::NoPadding<PlatformTag>(), "PlatformTag: empty type, no padding.");

} // namespace platformer

#endif // PLATFORMER_TYPES_HPP
