#include "Point3D.hpp"
#include "Math/Math.hpp"

Point3D::Point3D(const float &x, const float &y, const float &z) : x(x), y(y), z(z)
{
}

Point3D Point3D::Normalize() const
{
    const auto denominator = Math::Power(x * x + y * y + z * z, 0.5f);
    return *this / denominator;
}

Point3D Point3D::Cross(const Point3D &other) const
{
    return Point3D(y * other.z - z * other.y, x * other.z - z * other.x, x * other.y - y * other.x);
}

Point3D &Point3D::operator=(const Point3D &other)
{
    x = other.x;
    y = other.y;
    z = other.z;
    return *this;
}

Point3D Point3D::operator+(const Point3D &other) const
{
    return Point3D(x + other.x, y + other.y, z + other.z);
}

Point3D &Point3D::operator+=(const Point3D &other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

Point3D Point3D::operator-(const Point3D &other) const
{
    return Point3D(x - other.x, y - other.y, z - other.z);
}

Point3D &Point3D::operator-=(const Point3D &other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

Point3D Point3D::operator-() const
{
    return Point3D(-x, -y, -z);
}

Point3D Point3D::operator*(float scaler) const
{
    return Point3D(x * scaler, y * scaler, z * scaler);
}

Point3D Point3D::operator*=(float scaler)
{
    x *= scaler;
    y *= scaler;
    z *= scaler;
    return *this;
}

Point3D Point3D::operator/(float scaler) const
{
    return Point3D(x / scaler, y / scaler, z / scaler);
}

Point3D Point3D::operator/=(float scaler)
{
    x /= scaler;
    y /= scaler;
    z /= scaler;
    return *this;
}
