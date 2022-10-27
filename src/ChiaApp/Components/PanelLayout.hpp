#ifndef PANEL_LAYOUT_HPP
#define PANEL_LAYOUT_HPP

#include "Display/GUI/GUILayout.hpp"
#include "TopPanelBar.hpp"

class PanelLayout : public GUILayout
{
  public:
    static const unsigned long TopBarHeight;

    PanelLayout(const Point2D &windowSize);
};

#endif // PANEL_LAYOUT_HPP
