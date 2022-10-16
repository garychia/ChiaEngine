#include "Container.hpp"

Container::Container(const Point2D &windowSize, const Border &border)
    : IGUI(windowSize, border), pChildren()
{
}

Container::~Container()
{
    for (size_t i = 0; i < pChildren.Length(); i++)
        delete pChildren[i];
}

void Container::SetWindowSize(const Point2D &newSize)
{
    for (size_t i = 0; i < pChildren.Length(); i++)
        pChildren[i]->SetWindowSize(newSize);
    windowSize = newSize;
    OnResized();
}

void Container::OnResized()
{
    for (size_t i = 0; i < pChildren.Length(); i++)
        pChildren[i]->OnResized();
    IGUI::OnResized();
}
