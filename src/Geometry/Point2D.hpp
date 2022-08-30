#ifndef POINT2D_HPP
#define POINT2D_HPP

template <class T>
struct Point2D
{
    T x;
    T y;

    Point2D(T x, T y) : x(x), y(y) {}

    Point2D operator+(const Point2D &other) const
    {
        return Point2D(x + other.x, y + other.y);
    }
};

#endif