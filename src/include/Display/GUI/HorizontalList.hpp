#ifndef HORIZONTAL_LIST_HPP
#define HORIZONTAL_LIST_HPP

#include "Display/GUI/PanelPane.hpp"

// 水平排列的 pane(GUILayer 子類)。ADR-0001 D3:基底改為 PanelPane,
// 讓任何水平列表都是可註冊的 named pane(工具列 = TopBar dock 的 pane)。
class HorizontalList : public PanelPane
{
  private:
    bool resizable;

    void ArrangeChildren();

  public:
    HorizontalList(const Point2D &windowSize, const Border &border, const String &title = String(),
                   PanelDock dock = PanelDock::LeftDock, bool resizable = false);

    template <class GUIType, class... Args> SharedPtr<GUIType> AddComponent(Args... args)
    {
        auto child = PanelPane::AddComponent<GUIType, Args...>(args...);
        ArrangeChildren();
        return child;
    }
};

#endif