#ifndef LINE2D_HPP
#define LINE2D_HPP

#include "Point2D.hpp"

template <class T>
struct Line2D
{
    Point2D<T> start;
    Point2D<T> end;

    template <class Point>
    Line2D(Point &&start, Point &&end) : start(std::forward<Point>(start)), end(std::forward<Point>(end)) {}
};

#endif