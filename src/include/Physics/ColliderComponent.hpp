#ifndef PHYSICS_COLLIDER_COMPONENT_HPP
#define PHYSICS_COLLIDER_COMPONENT_HPP

#include <cstdint>

// 每個 Entity 一個 ColliderComponent(單型別,單 ComponentPool,單一 SoA 表)。
// - kind 是 identity tag(uint8),data[7] 是扁平 payload(POD、trivially copyable、
//   固定大小)→ World::Hash() 的 raw-byte FNV 位元級穩定。
// - AABB : data[0..2] = center(x,y,z), data[3..5] = halfExtents(x,y,z)
// - Sphere: data[0..2] = center(x,y,z), data[3] = radius
// 世界空間絕對 bounds:Sim 層沒有 Transform,pose 屬於 collider 自己。
struct ColliderComponent
{
    enum class Kind : uint8_t
    {
        AABB = 0,
        Sphere = 1,
    };

    Kind kind;
    float data[7];
};

#endif // PHYSICS_COLLIDER_COMPONENT_HPP
