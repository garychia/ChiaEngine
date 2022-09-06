#ifndef LINE3D_HPP
#define LINE3D_HPP

#include "Point3D.hpp"

template <class T>
struct Line3D
{
    Point3D<T> start;
    Point3D<T> end;

    template <class Point>
    Line3D(Point &&start, Point &&end) : start(std::forward<Point>(start)), end(std::forward<Point>(end)) {}

    Line3D(const Line3D &line) = default;

    Line3D(Line3D &&line) : start(std::move(line.start)), end(std::move(line.end)) {}
};

#endif