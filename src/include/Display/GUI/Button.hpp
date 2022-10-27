#ifndef BUTTON_HPP
#define BUTTON_HPP

#include "IInteractable.hpp"
#include "System/Operation/Event.hpp"

class Button : public IInteractable
{
  public:
    Event<void()> pressEvent;

    Event<void()> releaseEvent;

    Event<void()> clickEvent;

    Event<void()> hoverEvent;

    Button(const Point2D &windowSize, const Border &border);

    virtual void OnMouseDown(const Point2D &coordinates) override;

    virtual void OnMouseUp(const Point2D &coordinates) override;

    virtual void OnClicked(const Point2D &coordinates) override;

    virtual void OnHovered(const Point2D &coordinates) override;
};

#endif // BUTTON_HPP
