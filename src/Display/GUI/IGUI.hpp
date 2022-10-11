#ifndef IGUI_HPP
#define IGUI_HPP

#include "Border.hpp"
#include "Display/WindowInfo.hpp"
#include "Geometry/2D/Point2D.hpp"

class IGUI
{
  protected:
    WindowInfo *pWindowInfo;

    Point2D position;

    Border border;

    IGUI *pParent;

  public:
    IGUI(WindowInfo *pWindowInfo, const Point2D &position = Point2D(), const Border &border = Border(),
         IGUI *pParent = nullptr);

    void SetPosition(const Point2D &newPosition);

    void SetParent(IGUI *pNewParent);

    void SetWindowInfo(WindowInfo *pInfo);

    IGUI *GetParent();

    const IGUI *GetParent() const;

    Border &GetBorder();

    const Border &GetBorder() const;

    virtual bool IsBorderFixed() const;

    virtual bool IsBorderRelative() const;

    virtual bool IsBorderResizable() const;

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
};

#endif
