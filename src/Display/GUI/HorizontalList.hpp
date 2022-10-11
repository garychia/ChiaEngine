#ifndef HORIZONTAL_LIST_HPP
#define HORIZONTAL_LIST_HPP

#include "Container.hpp"

class HorizontalList : public Container
{
  private:
    void ArrangeChildren();

    void ResizeChildren();

  public:
    HorizontalList(WindowInfo *pWindowInfo, const Point2D &position = Point2D(), const Border &border = Border(),
                   IGUI *pParent = nullptr);

    virtual ~HorizontalList();

    template <class GUIType, class... Args> void AddChild(Args... args)
    {
        Container::AddChild<GUIType, Args...>(args...);
        ArrangeChildren();
    }
};

#endif
