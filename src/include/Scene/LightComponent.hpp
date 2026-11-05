#ifndef LIGHT_COMPONENT_HPP
#define LIGHT_COMPONENT_HPP

#include <cstdint>

#include "Geometry/3D/Point3D.hpp"

// 節點光源元件。type 為 directional 時 radius 無效；type 為 point 時
// radius 表示點光影響範圍。color 為 RGB，intensity 為光強乘數。
// 整體 trivially copyable、無 padding → 可安全進 ComponentPool::Hash。
struct LightComponent
{
    enum Type : uint32_t
    {
        Directional = 0,
        Point = 1,
    };

    Type type = Directional;
    Point3D color = Point3D(1, 1, 1);
    float intensity = 1.0f;
    float radius = 10.0f;
    bool castsShadow = false;

    LightComponent() = default;
    explicit LightComponent(Type type_) : type(type_) {}
};

#endif // LIGHT_COMPONENT_HPP
