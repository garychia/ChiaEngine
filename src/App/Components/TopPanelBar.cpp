#include "TopPanelBar.hpp"

#include "Display/GUI/Button.hpp"

TopPanelBar::TopPanelBar(const Point2D &windowSize, float height)
    : HorizontalList(windowSize, Border(0.f, 0.f, {1.f, true}, height))
{
    SetColor(Color(1, 1, 1));
    auto pButton1 = HorizontalList::AddComponent<Button>(windowSize, Border(0.f, 0.f, 90.f, height));
    pButton1->SetColor(Color(1, 1, 0));
    auto pButton2 = HorizontalList::AddComponent<Button>(windowSize, Border(0.f, 0.f, 90.f, height));
    pButton2->SetColor(Color(1, 0, 0));
}

TopPanelBar::~TopPanelBar()
{
}
