#ifndef PHYSICS_AABB_COLLIDER_HPP
#define PHYSICS_AABB_COLLIDER_HPP

#include "Geometry/3D/Point3D.hpp"

// AABB 碰撞形狀:軸對齊盒,世界空間,固定中心 + 半延展(SOA-friendly POD)。
// Sim 層確定性:無繼承 Transform(Sim 層沒有 Transform),pose 直接長在小型狀上。
// 邊界包含式:相切也算 overlap。
struct AABBCollider
{
    Point3D center;
    Point3D halfExtents;
};

#endif // PHYSICS_AABB_COLLIDER_HPP
