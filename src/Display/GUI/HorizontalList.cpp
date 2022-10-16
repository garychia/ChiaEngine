#include "HorizontalList.hpp"

#include "Math/Math.hpp"

void HorizontalList::ArrangeChildren()
{
    auto currentPos = GetTopLeftPosition() + Point2D(GetPaddingLeft(), GetPaddingTop());
    float totalWidth = 0.f;
    float maxHeight = 0.f;
    for (size_t i = 0; i < this->pChildren.Length(); i++)
    {
        auto pChild = this->pChildren[i];
        totalWidth += pChild->GetWidth();
        maxHeight = Math::Max(maxHeight, GetPaddingTop() + GetPaddingBottom() + pChild->GetHeight());
        if (!i)
            totalWidth += GetPaddingLeft();
        if (i + 1 == this->pChildren.Length())
            totalWidth += GetPaddingRight();

        if (i)
            totalWidth += pChild->GetMarginLeft();
        if (i + 1 < this->pChildren.Length())
            totalWidth += pChild->GetMarginRight();

        pChild->SetPosition({currentPos.x, false}, {currentPos.y, false});
        currentPos.x = totalWidth;
        if (i + 1 < this->pChildren.Length())
            currentPos.x += this->pChildren[i + 1]->GetMarginLeft();
    }
    if (resizable)
    {
        border.width = totalWidth;
        border.height = maxHeight;
    }
    OnResized();
}

HorizontalList::HorizontalList(const Point2D &windowSize, const Border &border, bool resizable)
    : Container(windowSize, border), resizable(resizable)
{
}

HorizontalList::~HorizontalList()
{
}
