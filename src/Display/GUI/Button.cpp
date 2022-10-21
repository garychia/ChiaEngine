#include "Button.hpp"

Button::Button(const Point2D &windowSize, const Border &border)
    : IInteractable(windowSize, border), pressEvent(), releaseEvent(), clickEvent(), hoverEvent()
{
}

void Button::OnMouseDown(const Point2D &coordinates)
{
    if (!WithIn(coordinates))
        return;
    pressEvent.Invoke();
}

void Button::OnMouseUp(const Point2D &coordinates)
{
    if (!WithIn(coordinates))
        return;
    releaseEvent.Invoke();
}

void Button::OnClicked(const Point2D &coordinates)
{
    if (!WithIn(coordinates))
        return;
    clickEvent.Invoke();
}

void Button::OnHovered(const Point2D &coordinates)
{
    if (!WithIn(coordinates))
        return;
    hoverEvent.Invoke();
}
