#ifndef MOUSE_INFO_HPP
#define MOUSE_INFO_HPP

#include "Geometry/2D/Point2D.hpp"

enum MouseStatus
{
    LeftButtonDown,
    LeftButtonUp,
    RightButtonDown,
    RightButtonUp,
    Move,
    WheelRotated,
    WheelPressed
};

struct MouseInfo
{
    MouseStatus status;

    bool leftButtonDown;

    bool rightButtonDown;

    bool wheelButtonDown;

    Point2D lastLeftButtonDownPosition;

    Point2D lastLeftButtonUpPosition;

    Point2D lastRightButtonDownPosition;

    Point2D lastRightButtonUpPosition;

    Point2D lastMousePosition;

    Point2D currentPosition;

    int wheelDistance;

    MouseInfo();
};

#endif
