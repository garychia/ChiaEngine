#ifndef IGUI_HPP
#define IGUI_HPP

#include "Border.hpp"
#include "Data/Pointers.hpp"
#include "Geometry/2D/Point2D.hpp"
#include "Geometry/Primitives.hpp"

class IGUI
{
  protected:
    Point2D windowSize;

    Border border;

    SharedPtr<class Rectangle> renderArea;

  public:
    IGUI(const Point2D &windowSize, const Border &border);

    virtual void SetPosition(const Border::Length &newX, const Border::Length &newY = Border::Length());

    virtual void SetWindowSize(const Point2D &newSize);

    Border &GetBorder();

    const Border &GetBorder() const;

    virtual float GetPaddingTop() const;

    virtual float GetPaddingBottom() const;

    virtual float GetPaddingLeft() const;

    virtual float GetPaddingRight() const;

    virtual float GetMarginTop() const;

    virtual float GetMarginBottom() const;

    virtual float GetMarginLeft() const;

    virtual float GetMarginRight() const;

    virtual Point2D GetTopLeftPosition() const;

    virtual float GetWidth() const;

    virtual float GetHeight() const;

    virtual void OnResized();

    virtual void SetColor(const Color &newColor);
};

#endif
