#ifndef PHYSICS_COLLIDER_HANDLER_HPP
#define PHYSICS_COLLIDER_HANDLER_HPP

#include "Physics/AABBCollider.hpp"
#include "Physics/ColliderComponent.hpp"
#include "Physics/Overlap.hpp"
#include "Physics/SphereCollider.hpp"

// ColliderComponent -> 具體形狀的薄轉換層:把扁平 float[7] payload 讀成
// AABBCollider / SphereCollider(純函式,無狀態,確定性)。
namespace ColliderHandler
{

inline AABBCollider GetAABB(const ColliderComponent &component)
{
    return AABBCollider{Point3D(component.data[0], component.data[1], component.data[2]),
                        Point3D(component.data[3], component.data[4], component.data[5])};
}

inline SphereCollider GetSphere(const ColliderComponent &component)
{
    return SphereCollider{Point3D(component.data[0], component.data[1], component.data[2]), component.data[3]};
}

inline bool Overlaps(const ColliderComponent &a, const ColliderComponent &b)
{
    switch (a.kind)
    {
    case ColliderComponent::Kind::AABB:
        switch (b.kind)
        {
        case ColliderComponent::Kind::AABB:
            return ::Overlaps(GetAABB(a), GetAABB(b));
        case ColliderComponent::Kind::Sphere:
            return ::Overlaps(GetAABB(a), GetSphere(b));
        }
        break;
    case ColliderComponent::Kind::Sphere:
        switch (b.kind)
        {
        case ColliderComponent::Kind::AABB:
            return ::Overlaps(GetSphere(a), GetAABB(b));
        case ColliderComponent::Kind::Sphere:
            return ::Overlaps(GetSphere(a), GetSphere(b));
        }
        break;
    }
    return false;
}

} // namespace ColliderHandler

#endif // PHYSICS_COLLIDER_HANDLER_HPP
