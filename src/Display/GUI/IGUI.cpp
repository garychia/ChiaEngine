#include "IGUI.hpp"

#include "Math/Math.hpp"

IGUI::IGUI(const Point2D &windowSize, const Border &border) : windowSize(windowSize), border(border)
{
    renderArea = SharedPtr<Rectangle>::Construct();
    OnResized();
}

void IGUI::SetPosition(const Border::Length &newX, const Border::Length &newY)
{
    border.xPos = newX;
    border.yPos = newY;
    auto &transform = renderArea->GetTransformation();
    const auto newPosition = GetTopLeftPosition();
    transform.position.x = (newPosition.x + GetWidth() / 2) / windowSize.x * 2 - 1;
    transform.position.y = -(newPosition.y + GetHeight() / 2) / windowSize.y * 2 + 1;
}

void IGUI::SetWindowSize(const Point2D &newSize)
{
    windowSize = newSize;
    OnResized();
}

Border &IGUI::GetBorder()
{
    return border;
}

const Border &IGUI::GetBorder() const
{
    return border;
}

float IGUI::GetPaddingTop() const
{
    if (border.padding.top.IsRelative())
        return border.padding.top * windowSize.y;
    return border.padding.top;
}

float IGUI::GetPaddingBottom() const
{
    if (border.padding.bottom.IsRelative())
        return border.padding.bottom * windowSize.y;
    return border.padding.bottom;
}

float IGUI::GetPaddingLeft() const
{
    if (border.padding.left.IsRelative())
        return border.padding.left * windowSize.x;
    return border.padding.left;
}

float IGUI::GetPaddingRight() const
{
    if (border.padding.right.IsRelative())
        return border.padding.right * windowSize.x;
    return border.padding.right;
}

float IGUI::GetMarginTop() const
{
    if (border.margin.top.IsRelative())
        return border.margin.top * windowSize.y;
    return border.margin.top;
}

float IGUI::GetMarginBottom() const
{
    if (border.margin.bottom.IsRelative())
        return border.margin.bottom * windowSize.y;
    return border.margin.bottom;
}

float IGUI::GetMarginLeft() const
{
    if (border.margin.left.IsRelative())
        return border.margin.left * windowSize.x;
    return border.margin.left;
}

float IGUI::GetMarginRight() const
{
    if (border.margin.right.IsRelative())
        return border.margin.right * windowSize.x;
    return border.margin.right;
}

Point2D IGUI::GetTopLeftPosition() const
{
    const float x = border.xPos.IsRelative() ? border.xPos * windowSize.x : border.xPos;
    const float y = border.yPos.IsRelative() ? border.yPos * windowSize.y : border.yPos;
    return Point2D(x, y);
}

float IGUI::GetWidth() const
{
    return border.width.IsRelative() ? border.width * windowSize.x : border.width;
}

float IGUI::GetHeight() const
{
    return border.height.IsRelative() ? border.height * windowSize.y : border.height;
}

void IGUI::OnResized()
{
    SetPosition(border.xPos, border.yPos);
    renderArea->Scale(GetWidth() / windowSize.x * 2, GetHeight() / windowSize.y * 2);
}

SharedPtr<IRenderable> IGUI::GetRenderable()
{
    return renderArea;
}

const SharedPtr<IRenderable> IGUI::GetRenderable() const
{
    return renderArea;
}
