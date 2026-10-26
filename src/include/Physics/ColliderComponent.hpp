#ifndef PHYSICS_COLLIDER_COMPONENT_HPP
#define PHYSICS_COLLIDER_COMPONENT_HPP

#include "Data/Padding.hpp"

#include <cstdint>

// 每個 Entity 一個 ColliderComponent(單型別,單 ComponentPool,單一 SoA 表)。
// - kind 是 identity tag,data[7] 是扁平 payload(POD、trivially copyable、固定大小)
//   → World::Hash() 的 raw-byte FNV 位元級穩定。
// - AABB : data[0..2] = center(x,y,z), data[3..5] = halfExtents(x,y,z)
// - Sphere: data[0..2] = center(x,y,z), data[3] = radius
// 世界空間絕對 bounds:Sim 層沒有 Transform,pose 屬於 collider 自己。
struct ColliderComponent
{
    // 底層型別刻意用 uint32_t 而不是 uint8_t:uint8_t 的 Kind 後面會被 ABI 補 3 byte
    // 隱含 padding(實測 sizeof 32 但成員只佔 29),而隱含 padding 的值 unspecified
    // → Hash() 逐位元組讀它時,clang -O2 會把來源物件的垃圾帶進池子、gcc -O2 恰好
    // 清零,同值的兩個 world hash 就不同(#129 / #142)。
    // 加寬後所有 32 byte 都是成員,kind 以外的 3 byte 由 0 填滿,語意不變、sizeof 不變。
    enum class Kind : uint32_t
    {
        AABB = 0,
        Sphere = 1,
    };

    Kind kind;
    float data[7];
};

CHIA_PADDING_FREE(ColliderComponent, &ColliderComponent::kind, &ColliderComponent::data);

#endif // PHYSICS_COLLIDER_COMPONENT_HPP
