#ifndef WORLD_TRANSFORM_COMPONENT_HPP
#define WORLD_TRANSFORM_COMPONENT_HPP

#include "Geometry/3D/Point3D.hpp"

// 每個節點的世界空間 TRS(組合後的 world transform)。
// 每次 FixedUpdate 依確定性 pre-order 全樹重算(設計 D2-A、§4):
// - root 的 world = local
// - child 的 world = Compose(parentWorld, local)
// 與 local TransformComponent 同 layout 但是不同型別 → 各自獨立 pool。
// View 側投影器讀這個 component 寫回 IRenderable(Sim 不觸碰 View 型別)。
struct WorldTransformComponent
{
    Point3D position;
    Point3D rotation;
    Point3D scale;
};

#endif // WORLD_TRANSFORM_COMPONENT_HPP