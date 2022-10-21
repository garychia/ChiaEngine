#include "IInteractable.hpp"

IInteractable::IInteractable(const Point2D &windowSize, const Border &border) : IGUI(windowSize, border)
{
}

bool IInteractable::WithIn(const Point2D &coordinates) const
{
    const auto topLeft = GetTopLeftPosition();
    const auto width = GetWidth();
    const auto height = GetHeight();
    return coordinates.x >= topLeft.x && coordinates.x <= topLeft.x + width && coordinates.y >= topLeft.y &&
           coordinates.y <= topLeft.y + height;
}

void IInteractable::OnMouseDown(const Point2D &coordinates)
{
}

void IInteractable::OnMouseUp(const Point2D &coordinates)
{
}

void IInteractable::OnClicked(const Point2D &coordinates)
{
}

void IInteractable::OnHovered(const Point2D &coordinates)
{
}
