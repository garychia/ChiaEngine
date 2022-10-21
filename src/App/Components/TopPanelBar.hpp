#ifndef TOP_PANEL_BAR_HPP
#define TOP_PANEL_BAR_HPP

#include "Display/GUI/HorizontalList.hpp"
#include "Geometry/2D/Point2D.hpp"

class TopPanelBar : public HorizontalList
{
  public:
    TopPanelBar(const Point2D &windowSize, float height = 30.f);
    ~TopPanelBar();
};

#endif // TOP_PANEL_BAR_HPP
