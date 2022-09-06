#ifndef POINT2D_HPP
#define POINT2D_HPP

template <class T>
struct Point2D
{
    T x;
    T y;

    template <class Component>
    Point2D(Component &&x, Component &&y) : x(std::forward<Component>(x)), y(std::forward<Component>(y)) {}

    Point2D(const Point2D &point) = default;

    Point2D(Point2D &&point) : x(std::move(point.x)), y(std::move(point.y)) {}

    Point2D operator+(const Point2D &other) const
    {
        return Point2D(x + other.x, y + other.y);
    }

    Point2D operator-(const Point2D &other) const
    {
        return Point2D(x - other.x, y - other.y);
    }
};

#endif