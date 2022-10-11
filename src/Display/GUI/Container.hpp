#ifndef CONTAINER_HPP
#define CONTAINER_HPP

#include "Data/DynamicArray.hpp"
#include "IGUI.hpp"

class Container : public IGUI
{
  protected:
    DynamicArray<IGUI *> pChildren;

  public:
    Container(WindowInfo *pWindowInfo, const Point2D &position = Point2D(), const Border &border = Border(),
              IGUI *pParent = nullptr);

    virtual ~Container();

    template <class GUIType, class... Args> void AddChild(Args... args)
    {
        auto *newChild = new GUIType(args...);
        newChild->SetWindowInfo(pWindowInfo);
        newChild->SetParent(this);
        pChildren.Append(newChild);
    }
};

#endif
