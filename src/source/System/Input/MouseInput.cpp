#include "System/Input/MouseInput.hpp"
#include "System/Input/InputHandler.hpp"

MouseInput::MouseInput() : info()
{
}

MouseInput &MouseInput::GetSingleton()
{
    static MouseInput singleton;
    return singleton;
}

void MouseInput::OnMouseLeftButtonDown(float xPosition, float yPosition)
{
    info.status = MouseStatus::LeftButtonDown;
    info.leftButtonDown = true;
    info.lastLeftButtonDownPosition.x = xPosition;
    info.lastLeftButtonDownPosition.y = yPosition;
}

void MouseInput::OnMouseLeftButtonUp(float xPosition, float yPosition)
{
    info.status = MouseStatus::LeftButtonUp;
    info.leftButtonDown = false;
    info.lastLeftButtonUpPosition.x = xPosition;
    info.lastLeftButtonUpPosition.y = yPosition;
}

void MouseInput::OnMouseRightButtonDown(float xPosition, float yPosition)
{
    info.status = MouseStatus::RightButtonDown;
    info.rightButtonDown = true;
    info.lastRightButtonDownPosition.x = xPosition;
    info.lastRightButtonDownPosition.y = yPosition;
}

void MouseInput::OnMouseRightButtonUp(float xPosition, float yPosition)
{
    info.status = MouseStatus::RightButtonUp;
    info.rightButtonDown = false;
    info.lastRightButtonUpPosition.x = xPosition;
    info.lastRightButtonUpPosition.y = yPosition;
}

void MouseInput::OnMouseMove(float xPosition, float yPosition)
{
    info.status = MouseStatus::Move;
    if (!lastPositionSet)
    {
        lastPositionSet = true;
        info.lastMousePosition.x = xPosition;
        info.lastMousePosition.y = yPosition;
        info.currentPosition.x = xPosition;
        info.currentPosition.y = yPosition;
    }
    else
    {
        info.lastMousePosition.x = info.currentPosition.x;
        info.lastMousePosition.y = info.currentPosition.y;
        info.currentPosition.x = xPosition;
        info.currentPosition.y = yPosition;
    }
}

void MouseInput::OnWheelRotated(int distance)
{
    info.status = MouseStatus::WheelRotated;
    info.wheelDistance = distance;
}

void MouseInput::OnWindowLoseFocus()
{
    info = MouseInfo();
}

const MouseInfo &MouseInput::GetMouseInfo() const
{
    return info;
}
