#ifndef TRANSFORM_MATH_HPP
#define TRANSFORM_MATH_HPP

#include "Geometry/3D/Point3D.hpp"
#include "Math/Math.hpp"
#include "Scene/TransformComponent.hpp"
#include "Scene/WorldTransformComponent.hpp"

// 純函式 local→world 組合(D2-A,分解 TRS)。
// 與 PixelOpenGLHelper::BuildWorldMatrix 慣例一致:T·Rx·Ry·Rz·S。
// rotation 單位 = degree;內部以 ToRadians 轉弧度。
// 註:worldRot 是歐拉加法近似(§8 C1 已知受限);translation/scale 精確。

namespace TransformMath
{

// 逐分量乘(Point3D 沒有 operator*(Point3D),這裡補)。
inline Point3D ScaleMul(const Point3D &a, const Point3D &b)
{
    return Point3D(a.x * b.x, a.y * b.y, a.z * b.z);
}

// R(rotDegree) 旋轉向量 v:先應用 Rx,再 Ry,再 Rz(與 BuildWorldMatrix 的 T·Rx·Ry·Rz·S 同 order)。
inline Point3D RotateEuler(const Point3D &v, const Point3D &rotDegree)
{
    const float rx = Math::ToRadians(rotDegree.x);
    const float ry = Math::ToRadians(rotDegree.y);
    const float rz = Math::ToRadians(rotDegree.z);

    const float cx = Math::Cosine(rx), sx = Math::Sine(rx);
    const float cy = Math::Cosine(ry), sy = Math::Sine(ry);
    const float cz = Math::Cosine(rz), sz = Math::Sine(rz);

    // Rx
    float x = v.x;
    float y = cx * v.y - sx * v.z;
    float z = sx * v.y + cx * v.z;

    // Ry
    float x2 = cy * x + sy * z;
    float z2 = -sy * x + cy * z;
    x = x2;
    z = z2;

    // Rz
    float x3 = cz * x - sz * y;
    float y3 = sz * x + cz * y;
    y = y3;
    x = x3;

    return Point3D(x, y, z);
}

// child local(l) 連著 parent 世界(P): world = Compose(parentWorld, local)
    inline WorldTransformComponent Compose(const WorldTransformComponent &parentWorld, const TransformComponent &local)
    {
        WorldTransformComponent result;
        result.scale = ScaleMul(parentWorld.scale, local.scale);
        result.position = parentWorld.position +
                          RotateEuler(ScaleMul(parentWorld.scale, local.position), parentWorld.rotation);
        result.rotation = Point3D(parentWorld.rotation.x + local.rotation.x,
                                  parentWorld.rotation.y + local.rotation.y,
                                  parentWorld.rotation.z + local.rotation.z);
        return result;
    }

} // namespace TransformMath

#endif // TRANSFORM_MATH_HPP