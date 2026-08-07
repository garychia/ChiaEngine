#include "Display/GUI/Button.hpp"

Button::Button(const Point2D &windowSize, const Border &border)
    : IInteractable(windowSize, border), label(), fontSize(16.0f), textColor(1.0f, 1.0f, 1.0f),
      pressEvent(), releaseEvent(), clickEvent(), hoverEvent()
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

void Button::SetLabel(const String &text)
{
    label = text;
}

const String &Button::GetLabel() const
{
    return label;
}

void Button::SetFontSize(float size)
{
    fontSize = size;
}

float Button::GetFontSize() const
{
    return fontSize;
}

void Button::SetTextColor(const Color &color)
{
    textColor = color;
}

const Color &Button::GetTextColor() const
{
    return textColor;
}
