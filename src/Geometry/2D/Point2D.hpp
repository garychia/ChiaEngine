#ifndef POINT2D_HPP
#define POINT2D_HPP

#include "Types/Types.hpp"

struct Point2D
{
    float x;
    float y;

    Point2D(const float &x = 0.f, const float &y = 0.f);

    Point2D &operator=(const Point2D &other);

    Point2D operator+(const Point2D &other) const;

    Point2D operator-(const Point2D &other) const;
};

#endif