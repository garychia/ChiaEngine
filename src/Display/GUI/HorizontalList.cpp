#include "HorizontalList.hpp"

#include "Math/Math.hpp"

void HorizontalList::ArrangeChildren()
{
    auto currentPos = GetTopLeftPosition() + Point2D(GetPaddingLeft(), GetPaddingTop());
    float totalWidth = 0.f;
    float maxHeight = 0.f;
    for (size_t i = 0; i < this->pComponents.Length(); i++)
    {
        SharedPtr<IGUI> pChild = this->pComponents[i];
        totalWidth += pChild->GetWidth();
        maxHeight = Math::Max(maxHeight, GetPaddingTop() + GetPaddingBottom() + pChild->GetHeight());
        if (!i)
            totalWidth += GetPaddingLeft();
        if (i + 1 == this->pComponents.Length())
            totalWidth += GetPaddingRight();

        if (i)
            totalWidth += pChild->GetMarginLeft();
        if (i + 1 < this->pComponents.Length())
            totalWidth += pChild->GetMarginRight();

        pChild->SetTopLeftPosition({currentPos.x, false}, {currentPos.y, false});
        currentPos.x = totalWidth;
        if (i + 1 < this->pComponents.Length())
        {
            SharedPtr<IGUI> pNextComponent = this->pComponents[i + 1];
            currentPos.x += pNextComponent->GetMarginLeft();
        }
    }
    if (resizable)
    {
        border.width = totalWidth;
        border.height = maxHeight;
    }
    OnWindowResized();
}

HorizontalList::HorizontalList(const Point2D &windowSize, const Border &border, bool resizable)
    : GUILayer(windowSize, border), resizable(resizable)
{
}
