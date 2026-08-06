#ifndef PHYSICS_OVERLAP_HPP
#define PHYSICS_OVERLAP_HPP

#include "Math/Math.hpp"
#include "Physics/AABBCollider.hpp"
#include "Physics/SphereCollider.hpp"

// AABB-AABB:分離軸定理(逐軸 |dx| <= hx1 + hx2)。
bool Overlaps(const AABBCollider &a, const AABBCollider &b)
{
    return Math::Abs(a.center.x - b.center.x) <= a.halfExtents.x + b.halfExtents.x &&
           Math::Abs(a.center.y - b.center.y) <= a.halfExtents.y + b.halfExtents.y &&
           Math::Abs(a.center.z - b.center.z) <= a.halfExtents.z + b.halfExtents.z;
}

// 球-球:用平方長度比較,避免 sqrt(確定性-friendly、更快)。
bool Overlaps(const SphereCollider &a, const SphereCollider &b)
{
    const float dx = a.center.x - b.center.x;
    const float dy = a.center.y - b.center.y;
    const float dz = a.center.z - b.center.z;
    const float radiusSum = a.radius + b.radius;
    return dx * dx + dy * dy + dz * dz <= radiusSum * radiusSum;
}

// AABB-球:球心 clamp 到 AABB 內部後,檢查此點是否落在球內(距離平方 <= r^2)。
// 球心在盒內時 clamp 是恒等 → 0 <= r^2 成立,即「包含」算 overlap。
bool Overlaps(const AABBCollider &box, const SphereCollider &sphere)
{
    const float boxMinX = box.center.x - box.halfExtents.x;
    const float boxMaxX = box.center.x + box.halfExtents.x;
    const float boxMinY = box.center.y - box.halfExtents.y;
    const float boxMaxY = box.center.y + box.halfExtents.y;
    const float boxMinZ = box.center.z - box.halfExtents.z;
    const float boxMaxZ = box.center.z + box.halfExtents.z;
    const float closestX = Math::Max(boxMinX, Math::Min(sphere.center.x, boxMaxX));
    const float closestY = Math::Max(boxMinY, Math::Min(sphere.center.y, boxMaxY));
    const float closestZ = Math::Max(boxMinZ, Math::Min(sphere.center.z, boxMaxZ));
    const float dx = sphere.center.x - closestX;
    const float dy = sphere.center.y - closestY;
    const float dz = sphere.center.z - closestZ;
    return dx * dx + dy * dy + dz * dz <= sphere.radius * sphere.radius;
}

bool Overlaps(const SphereCollider &sphere, const AABBCollider &box)
{
    return Overlaps(box, sphere);
}

#endif // PHYSICS_OVERLAP_HPP
