#ifndef POINT3D_HPP
#define POINT3D_HPP

#include "Data/Padding.hpp"
#include "Types/Types.hpp"

struct Point3D
{
    float x;
    float y;
    float z;

    Point3D(const float &x = 0, const float &y = 0, const float &z = 0);

    Point3D Normalize() const;

    Point3D Cross(const Point3D &other) const;

    Point3D &operator=(const Point3D &other);

    Point3D operator+(const Point3D &other) const;

    Point3D &operator+=(const Point3D &other);

    Point3D operator-(const Point3D &other) const;

    Point3D &operator-=(const Point3D &other);

    Point3D operator-() const;

    Point3D operator*(float scaler) const;

    Point3D operator*=(float scaler);

    Point3D operator/(float scaler) const;

    Point3D operator/=(float scaler);
};

// 這個型別會被放進 ComponentPool(Point3D 是 Ball/Paddle/Collider 的成員),
// 所以 World::Hash() 逐位元組讀它 → 必須無隱含 padding。見 #142。
CHIA_PADDING_FREE(Point3D, &Point3D::x, &Point3D::y, &Point3D::z);

#endif
