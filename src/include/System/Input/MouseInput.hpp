#ifndef MOUSE_INPUT_HPP
#define MOUSE_INPUT_HPP

#include "MouseInfo.hpp"

class MouseInput
{
  private:
    bool lastPositionSet = false;

    MouseInfo info;

    MouseInput();

  public:
    static MouseInput &GetSingleton();

    void OnMouseLeftButtonDown(float xPosition, float yPosition);

    void OnMouseLeftButtonUp(float xPosition, float yPosition);

    void OnMouseRightButtonDown(float xPosition, float yPosition);

    void OnMouseRightButtonUp(float xPosition, float yPosition);

    void OnMouseMove(float xPosition, float yPosition);

    void OnWheelRotated(int distance);

    void OnWindowLoseFocus();

    const MouseInfo &GetMouseInfo() const;
};

#endif
