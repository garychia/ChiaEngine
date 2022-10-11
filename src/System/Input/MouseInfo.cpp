#include "MouseInfo.hpp"

MouseInfo::MouseInfo()
    : status(MouseStatus::Move), leftButtonDown(false), rightButtonDown(false), wheelButtonDown(false),
      lastLeftButtonDownPosition(), lastLeftButtonUpPosition(), lastRightButtonDownPosition(),
      lastRightButtonUpPosition(), lastMousePosition(), currentPosition(), wheelDistance()
{
}
