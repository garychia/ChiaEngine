#ifndef CONTAINER_HPP
#define CONTAINER_HPP

#include "Data/DynamicArray.hpp"
#include "IGUI.hpp"

class Container : public IGUI
{
  protected:
    DynamicArray<IGUI *> pChildren;

  public:
    Container(const Point2D &windowSize, const Border &border);

    virtual ~Container();

    template <class GUIType, class... Args> IGUI &AddChild(Args... args)
    {
        IGUI *newChild = new GUIType(args...);
        newChild->SetWindowSize(windowSize);
        pChildren.Append(newChild);
        return *newChild;
    }

    virtual void SetWindowSize(const Point2D &newSize) override;

    virtual void OnResized() override;
};

#endif
