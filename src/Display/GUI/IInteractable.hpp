#ifndef IINTERACTABLE_HPP
#define IINTERACTABLE_HPP

#include "IGUI.hpp"

class IInteractable : public IGUI
{
  public:
    IInteractable(const Point2D &windowSize, const Border &border);

    virtual bool WithIn(const Point2D &coordinates) const;

    virtual void OnMouseDown(const Point2D &coordinates);

    virtual void OnMouseUp(const Point2D &coordinates);

    virtual void OnClicked(const Point2D &coordinates);
    
    virtual void OnHovered(const Point2D &coordinates);
};

#endif
