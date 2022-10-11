#ifndef POINT3D_HPP
#define POINT3D_HPP

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

#endif
