#ifndef HORIZONTAL_LIST_HPP
#define HORIZONTAL_LIST_HPP

#include "GUILayer.hpp"

class HorizontalList : public GUILayer
{
  private:
    bool resizable;

    void ArrangeChildren();

  public:
    HorizontalList(const Point2D &windowSize, const Border &border, bool resizable = false);

    virtual ~HorizontalList();

    template <class GUIType, class... Args> SharedPtr<IGUI> AddComponent(Args... args)
    {
        auto child = GUILayer::AddComponent<GUIType, Args...>(args...);
        ArrangeChildren();
        return child;
    }
};

#endif
