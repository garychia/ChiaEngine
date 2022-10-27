#include "Geometry/2D/Point2D.hpp"

Point2D::Point2D(const float &x, const float &y) : x(x), y(y)
{
}

Point2D &Point2D::operator=(const Point2D &other)
{
    x = other.x;
    y = other.y;
    return *this;
}

Point2D Point2D::operator+(const Point2D &other) const
{
    return Point2D(x + other.x, y + other.y);
}

Point2D Point2D::operator-(const Point2D &other) const
{
    return Point2D(x - other.x, y - other.y);
}
