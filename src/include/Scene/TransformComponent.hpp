#ifndef TRANSFORM_COMPONENT_HPP
#define TRANSFORM_COMPONENT_HPP

#include "Geometry/3D/Point3D.hpp"

// 節點的 local TRS(local transform):position / rotation(degree, Euler)/ scale。
// 3 × Point3D(各 12 bytes)純粹、trivially copyable、無 padding → ComponentPool::Hash
// 對 raw bytes 的 FNV 位元級穩定(架構 §2.2:元件必須 POD)。
// rotation 以 degree 儲存,與 OpenGLHelper::BuildWorldMatrix 的慣例一致。
struct TransformComponent
{
    Point3D position;
    Point3D rotation;
    Point3D scale;

    TransformComponent(const Point3D &position = Point3D(), const Point3D &rotation = Point3D(),
                       const Point3D &scale = Point3D(1, 1, 1))
        : position(position), rotation(rotation), scale(scale)
    {
    }
};

#endif // TRANSFORM_COMPONENT_HPP