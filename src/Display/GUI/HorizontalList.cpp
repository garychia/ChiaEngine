#include "HorizontalList.hpp"

#include "Math/Math.hpp"

void HorizontalList::ArrangeChildren()
{
    if (IsBorderResizable())
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
            if (i + 1 < this->pChildren.Length())
                totalWidth += GetPaddingRight();

            if (i)
                totalWidth += pChild->GetMarginLeft();
            if (i + 2 < this->pChildren.Length())
                totalWidth += pChild->GetMarginRight();

            pChild->SetPosition(currentPos);
            currentPos.x = totalWidth;
            if (i + 2 < this->pChildren.Length())
                currentPos.x += this->pChildren[i + 1]->GetMarginLeft();
        }
        totalWidth = pParent ? Math::Min(totalWidth, pParent->GetWidth()) : totalWidth;
        maxHeight = pParent ? Math::Min(maxHeight, pParent->GetHeight()) : maxHeight;
        border.width = totalWidth;
        border.height = maxHeight;
    }
    else
    {
        ResizeChildren();
    }
}

void HorizontalList::ResizeChildren()
{
    float resizableWidth = GetWidth();
    size_t nResizables = 0;
    for (size_t i = 0; i < pChildren.Length(); i++)
    {
        auto pChild = pChildren[i];
        if (pChild->IsBorderResizable())
            nResizables++;
        else
            resizableWidth -= pChildren[i]->GetWidth();
    }
    const float widthAvg = Math::Max(resizableWidth / nResizables, 5.f);
    for (size_t i = 0; i < pChildren.Length(); i++)
    {
        auto pChild = pChildren[i];
        if (pChild->IsBorderResizable())
            pChildren[i]->GetBorder().width = widthAvg;
    }
}

HorizontalList::HorizontalList(WindowInfo *pWindowInfo, const Point2D &position, const Border &border, IGUI *pParent)
    : Container(pWindowInfo, position, border, pParent)
{
}

HorizontalList::~HorizontalList()
{
}
