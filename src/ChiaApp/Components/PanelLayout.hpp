#ifndef PANEL_LAYOUT_HPP
#define PANEL_LAYOUT_HPP

#include "Display/GUI/GUILayout.hpp"
#include "Display/GUI/HierarchyRow.hpp"
#include "Scene/SceneSystem.hpp"
#include "TopPanelBar.hpp"

// 主視窗佈局:GUI 走 Frame 的唯一出口(P6)。
// 目前兩層:頂部 toolbar(TopPanelBar)+ 左側 hierarchy 側欄(#60 step 1)。
class PanelLayout : public GUILayout
{
  public:
    static const unsigned long TopBarHeight;

    static const unsigned long SidebarWidth;

    static const unsigned long RowHeight;

    PanelLayout(const Point2D &windowSize);

    // #60 step 1:以 SceneSystem 節點重建 hierarchy 列(側欄,indent = 深度)。
    // 呼叫時機:SceneWindow::Initialize 建立完 demo 節點之後(Panel::Initialize)。
    void BuildHierarchy(SceneSystem &scene);

    DynamicArray<SharedPtr<HierarchyRow>> &GetHierarchyRows();

  private:
    SharedPtr<GUILayer> pHierarchyLayer;

    DynamicArray<SharedPtr<HierarchyRow>> pHierarchyRows;
};

#endif // PANEL_LAYOUT_HPP
