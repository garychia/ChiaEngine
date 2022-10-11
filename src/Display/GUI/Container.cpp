#include "Container.hpp"

Container::Container(WindowInfo *pWindowInfo, const Point2D &position, const Border &border, IGUI *pParent)
    : IGUI(pWindowInfo, position, border, pParent), pChildren()
{
}

Container::~Container()
{
    for (size_t i = 0; i < pChildren.Length(); i++)
        delete pChildren[i];
}
