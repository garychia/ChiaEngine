#ifndef PANEL_LAYOUT_HPP
#define PANEL_LAYOUT_HPP

#include "Display/GUI/GUILayout.hpp"
#include "Display/GUI/HierarchyRow.hpp"
#include "Display/GUI/InspectorLayer.hpp"
#include "Display/GUI/PanelRegions.hpp" // PanelRegions + ComputePanelRegions + constants
#include "Geometry/2D/Point2D.hpp"
#include "Scene/SceneSystem.hpp"
#include "TopPanelBar.hpp"

class UndoStack; // forward decl (CreateInspector 參數)

// ADR-0001 D5-resize contract:所有 region constants + 計算邏輯集中在
// PanelRegions.hpp(headless-testable)。PanelLayout 的 static 只是轉發,
// 保留舊的 static const 介面(呼叫端相容)。

// 主視窗佈局:GUI 走 Frame 的唯一出口(P6)。
// 目前三層:頂部 toolbar(TopPanelBar)+ 左側 hierarchy 側欄(#60 step 1)
// + 右側 inspector(#68 step 2)。
class PanelLayout : public GUILayout
{
  public:
    static const unsigned long TopBarHeight;

    static const unsigned long SidebarWidth;

    static const unsigned long RowHeight;

    static const unsigned long InspectorWidth;

    PanelLayout(const Point2D &windowSize);

    // #60 step 1:以 SceneSystem 節點重建 hierarchy 列(側欄,indent = 深度)。
    // 呼叫時機:SceneWindow::Initialize 建立完 demo 節點之後(Panel::Initialize)。
    void BuildHierarchy(SceneSystem &scene);

    // #60 step 2:建立右側 Inspector 層,顯示/編輯選取 entity 的 TransformComponent。
    // 必須在 BuildHierarchy 之後(選取來自 hierarchy 列)呼叫一次。
    // pUndoStack:ADR-0001 D5 — inspector 按鈕把編輯 push 到 session 的 undo stack。
    void CreateInspector(SceneSystem &scene, UndoStack *pUndoStack);

    // ADR-0001 D5-resize:reposition all dock layers + viewport from computed regions.
    // Call from Panel::OnWindowResized (and once at init). Refreshes depths (#67
    // requirement after dynamic panel changes).
    void SetRegions(const Point2D &windowSize, const PanelRegions &regions);

    InspectorLayer *GetInspector();

    DynamicArray<SharedPtr<HierarchyRow>> &GetHierarchyRows();

  private:
    SharedPtr<GUILayer> pHierarchyLayer;

    SharedPtr<InspectorLayer> pInspector;

    DynamicArray<SharedPtr<HierarchyRow>> pHierarchyRows;
};

#endif // PANEL_LAYOUT_HPP
