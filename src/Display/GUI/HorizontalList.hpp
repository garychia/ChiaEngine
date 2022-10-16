#ifndef HORIZONTAL_LIST_HPP
#define HORIZONTAL_LIST_HPP

#include "Container.hpp"

class HorizontalList : public Container
{
  private:
    bool resizable;

    void ArrangeChildren();

  public:
    HorizontalList(const Point2D &windowSize, const Border &border, bool resizable = false);

    virtual ~HorizontalList();

    template <class GUIType, class... Args> IGUI &AddChild(Args... args)
    {
        IGUI &child = Container::AddChild<GUIType, Args...>(args...);
        ArrangeChildren();
        return child;
    }
};

#endif
