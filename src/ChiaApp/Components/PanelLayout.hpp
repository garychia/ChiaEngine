#ifndef PANEL_LAYOUT_HPP
#define PANEL_LAYOUT_HPP

#include "Display/GUI/GUILayout.hpp"
#include "Display/GUI/HierarchyRow.hpp"
#include "Display/GUI/InspectorLayer.hpp"
#include "Display/GUI/PanelPane.hpp"
#include "Display/GUI/PanelRegions.hpp" // PanelRegions + ComputePanelRegions + constants
#include "Geometry/2D/Point2D.hpp"
#include "Scene/SceneSystem.hpp"
#include "TopPanelBar.hpp"

class EditorSession; // forward decl (pane registry 由 session 擁有)

// ADR-0001 D5-resize contract:所有 region constants + 計算邏輯集中在
// PanelRegions.hpp(headless-testable)。PanelLayout 的 static 只是轉發,
// 保留舊的 static const 介面(呼叫端相容)。

// 主視窗佈局:GUI 走 Frame 的唯一出口(P6)。
// ADR-0001 D3:PanelLayout 變成 dock manager — 不再用 GetLayers()[0] 魔術 index
// + 成員 slot 手排三層;它持有 named dock(TopBar / LeftDock / RightDock),每個
// 可見表面是 PanelPane。pane registry 由 EditorSession 擁有(D4);這裡只負責
// 註冊(新面板 = 一個 RegisterPane 呼叫)+ SetRegions 依 dock 定位。
class PanelLayout : public GUILayout
{
  public:
    static const unsigned long TopBarHeight;

    static const unsigned long SidebarWidth;

    static const unsigned long RowHeight;

    static const unsigned long InspectorWidth;

    PanelLayout(const Point2D &windowSize, EditorSession &session);

    // ADR-0001 D3:註冊 named pane 到 session registry,並加入本 layout 渲染。
    // 重複 title 回傳既有 pane(不重複加入)。新面板 = 這一個呼叫,
    // SetRegions 之後即依 dock 定位 — 不需要 constructor + SetRegions 手術。
    PanelPane *RegisterPane(SharedPtr<PanelPane> pPane);

    // #60 step 1:以 SceneSystem 節點重建 hierarchy 列(左側 dock 的
    // "Hierarchy" pane,indent = 深度)。
    void BuildHierarchy(SceneSystem &scene);

    // #60 step 2:建立右側 dock 的 "Inspector" pane(消費選取的 entity),
    // 按鈕 push 到 session 的 undo stack。
    void CreateInspector(SceneSystem &scene);

    // ADR-0001 D5-resize:依每個 pane 的 dock assignment 重新定位。
    // Call from Panel::OnWindowResized (and once at init)。collapsed pane
    // 高度歸零。刷新 depths(#67 requirement after dynamic panel changes)。
    void SetRegions(const Point2D &windowSize, const PanelRegions &regions);

    InspectorLayer *GetInspector();

    DynamicArray<SharedPtr<HierarchyRow>> &GetHierarchyRows();

  private:
    EditorSession &mSession; // registry owner(D4)

    Point2D mWindowSize;

    DynamicArray<SharedPtr<HierarchyRow>> pHierarchyRows;

    // dock 區 → region 對應(依 pane 的 dock 取出對應 PanelRegions 欄位)。
    static Border RegionForDock(PanelDock dock, const PanelRegions &regions);
};

#endif // PANEL_LAYOUT_HPP