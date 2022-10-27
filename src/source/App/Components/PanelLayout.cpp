#include "App/Components/PanelLayout.hpp"

const unsigned long PanelLayout::TopBarHeight = 30;

PanelLayout::PanelLayout(const Point2D &windowSize) : GUILayout()
{
    auto pTopPanelBar = SharedPtr<GUILayer>::Construct<TopPanelBar>(windowSize);
    AddLayer(pTopPanelBar);
}
