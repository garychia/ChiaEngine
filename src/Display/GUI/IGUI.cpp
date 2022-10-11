#include "IGUI.hpp"

#include "Math/Math.hpp"

IGUI::IGUI(WindowInfo *pWindowInfo, const Point2D &position, const Border &border, IGUI *pParent)
    : pWindowInfo(pWindowInfo), position(position), border(border), pParent(pParent)
{
}

void IGUI::SetPosition(const Point2D &newPosition)
{
    position = newPosition;
}

void IGUI::SetParent(IGUI *pNewParent)
{
    pParent = pNewParent;
}

void IGUI::SetWindowInfo(WindowInfo *pInfo)
{
    pWindowInfo = pInfo;
}

IGUI *IGUI::GetParent()
{
    return pParent;
}

const IGUI *IGUI::GetParent() const
{
    return pParent;
}

Border &IGUI::GetBorder()
{
    return border;
}

const Border &IGUI::GetBorder() const
{
    return border;
}

bool IGUI::IsBorderFixed() const
{
    return border.sizeFixed;
}

bool IGUI::IsBorderRelative() const
{
    return border.relative;
}

bool IGUI::IsBorderResizable() const
{
    return !IsBorderFixed() && !IsBorderRelative();
}

float IGUI::GetPaddingTop() const
{
    if (border.relative)
        return border.padding.top * pWindowInfo->height;
    return border.padding.top;
}

float IGUI::GetPaddingBottom() const
{
    if (border.relative)
        return border.padding.bottom * pWindowInfo->height;
    return border.padding.bottom;
}

float IGUI::GetPaddingLeft() const
{
    if (border.relative)
        return border.padding.left * pWindowInfo->width;
    return border.padding.left;
}

float IGUI::GetPaddingRight() const
{
    if (border.relative)
        return border.padding.right * pWindowInfo->width;
    return border.padding.right;
}

float IGUI::GetMarginTop() const
{
    if (border.relative)
        return border.margin.top * pWindowInfo->height;
    return border.margin.top;
}

float IGUI::GetMarginBottom() const
{
    if (border.relative)
        return border.margin.bottom * pWindowInfo->height;
    return border.margin.bottom;
}

float IGUI::GetMarginLeft() const
{
    if (border.relative)
        return border.margin.left * pWindowInfo->width;
    return border.margin.left;
}

float IGUI::GetMarginRight() const
{
    if (border.relative)
        return border.margin.right * pWindowInfo->width;
    return border.margin.right;
}

Point2D IGUI::GetTopLeftPosition() const
{
    if (!pParent)
        return Point2D();
    const Point2D parentPosition = pParent->GetTopLeftPosition();
    return parentPosition + Point2D(pParent->GetPaddingLeft(), pParent->GetPaddingTop());
}

float IGUI::GetWidth() const
{
    if (border.sizeFixed || !border.relative)
        return border.width;
    return border.width * pWindowInfo->width;
}

float IGUI::GetHeight() const
{
    if (border.sizeFixed || !border.relative)
        return border.height;
    return border.height * pWindowInfo->height;
}
