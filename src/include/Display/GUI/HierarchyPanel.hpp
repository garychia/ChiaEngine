#ifndef HIERARCHY_PANEL_HPP
#define HIERARCHY_PANEL_HPP

#include "Data/DynamicArray.hpp"
#include "Display/GUI/HierarchyRow.hpp"
#include "Display/GUI/PanelPane.hpp"
#include "Scene/SceneSystem.hpp"

// 前置宣告(避免循環 include):EditorSession 與 InspectorLayer 僅持指標。
class EditorSession;
class InspectorLayer;

// ADR-0001 D3:Hierarchy 面板(LeftDock named pane)。以 SceneSystem 為資料來源,
// 遞迴遍歷節點樹,每個節點一個 HierarchyRow;點擊列 → EditorSession::Select() +
// 刷新 InspectorLayer。View 層不直接碰 World,全部經 SceneSystem seam 存取。
class HierarchyPanel : public PanelPane
{
  public:
    // pSession:點擊後呼叫 Select(entityIndex); pInspector:選取變更後呼叫 SetSelection()。
    // 皆可為 nullptr(測試 / 未接線無妨)。
    HierarchyPanel(const String &title, const Point2D &windowSize, const Border &border,
                   PanelDock dock, SceneSystem *pScene, EditorSession *pSession = nullptr,
                   InspectorLayer *pInspector = nullptr);
    ~HierarchyPanel();

    // 重建節點列表(場景節點增刪後呼叫):清除現有行,遞迴遍歷 SceneSystem,
    // 依深度縮排建立 HierarchyRow,並訂閱各行的 rowClicked。
    void Rebuild();

    // 每幀由 Panel::Render 呼叫:反映目前選取(可由 Inspector/外部改變)。
    void Update();

    // 測試用查詢。
    size_t GetNRows() const { return mRows.GetNElements(); }
    HierarchyRow *GetRow(uint32_t i) { return mRows[i].GetRaw(); }

  private:
    SceneSystem *mpScene;
    EditorSession *mpSession;
    InspectorLayer *mpInspector;
    DynamicArray<SharedPtr<HierarchyRow>> mRows;
    static const float RowHeight;
    static const float IndentPerDepth;

    // 遞迴建立指定 entity 及其子孫的行(depth 用於縮排)。
    void BuildSubtree(Entity e, uint32_t depth, uint32_t &rowIndex);

    // 某列被點擊:同步 EditorSession 選取,並刷新 Inspector。
    void OnRowClicked(Entity entity);
};

#endif // HIERARCHY_PANEL_HPP
