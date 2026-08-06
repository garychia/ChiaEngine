#ifndef PHYSICS_SPHERE_COLLIDER_HPP
#define PHYSICS_SPHERE_COLLIDER_HPP

#include "Geometry/3D/Point3D.hpp"

// 球體碰撞形狀:世界空間固定中心 + 半徑(POD)。
// 邊界包含式:半徑之和 == 距離也算 overlap。
struct SphereCollider
{
    Point3D center;
    float radius = 0.0f;
};

#endif // PHYSICS_SPHERE_COLLIDER_HPP
