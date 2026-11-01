#ifndef INSPECTOR_LAYER_HPP
#define INSPECTOR_LAYER_HPP

#include "Data/String.hpp"
#include "Display/GUI/Button.hpp"
#include "Display/GUI/EditorSession.hpp" // UndoStack + EditorSession(指針)
#include "Display/GUI/InspectorButton.hpp"
#include "Display/GUI/PanelPane.hpp"
#include "Display/GUI/Selection.hpp"
#include "Scene/SceneSystem.hpp"

// 前置宣告(EditorSession 完整定義在 .cpp 內使用)。EditorSession 只作指針持有,
// 不擁有;Panel/App 長時間持有。
class EditorSession;

// Editor inspector(#68 step 2 + #168 優先級 2):右側面板,顯示並編輯目前選取 entity 的
// TransformComponent(position / rotation / scale,各 3 軸)與綁定的材質(materialId)。
// 每軸一行:左側文字欄(目前值)+ 右側 + / - 兩顆調整鈕。材質區顯示目前 materialId,
// 並提供 Bind/Unbind 貼圖按鈕(從已載入貼圖列表選)。
// 邏輯全在 Sim/View 純資料層,可無頭測試(SceneSystem + InspectorLayer + EditorSession)。
class InspectorLayer : public PanelPane
{
  public:
    // ADR-0001 D3:Inspector 是 RightDock 的 named pane(title + dock)。
    // pSession:供材質綁定查詢(可為 nullptr,測試/未接線無妨)。
    InspectorLayer(const String &title, const Point2D &windowSize, const Border &border,
                   PanelDock dock, SceneSystem *pScene, UndoStack *pUndoStack,
                   EditorSession *pSession = nullptr);

    // 設定目前選取的 entity(由 Panel 在點擊 hierarchy 列時呼叫)。
    // ADR-0001 D2:直接引用 Panel 擁有的 Selection,避免雙份狀態。
    void SetSelection(const Selection *pSelection);

    // 每幀由 Panel::Render 呼叫:反映 selectedEntity / 外部 transform 改動。
    void Update();

    // 測試/按鈕共用:對 entity 的 TransformComponent 給定 axis 加上 sign * step。
    void ApplyEdit(uint32_t entityIndex, InspectorAxis axis, float sign);

    // 測試用:取 field row 陣列(只顯示文字的 9 顆)。
    SharedPtr<Button> *GetFieldRows();

    // 測試用:取材質標籤文字。
    const String &GetMaterialLabel() const { return mMaterialLabel; }

  private:
    static const float StepPosition;
    static const float StepRotation;
    static const float StepScale;
    static const float RowHeight;
    static const float ColLabelWidth;
    static const float BtnWidth;

    SceneSystem *pScene;
    UndoStack *pUndoStack; // ADR-0001 D5:按鈕 push 到 session 的 undo stack
    const Selection *pSelection; // ADR-0001 D2:由 Panel 擁有,Inspector 只讀
    EditorSession *mpSession; // ADR-0001 D4:材質綁定查詢(不擁有,可為 nullptr)

    // 9 個 field row(只顯示文字):index 0..8 對應 InspectorAxis 順序。
    SharedPtr<Button> pFieldRows[9];
    // 9 × 2 個 + / - 鈕(同樣順序)。
    SharedPtr<InspectorButton> pButtons[18];

    // 材質區(RowHeight 後的額外區塊)。
    SharedPtr<Button> pMaterialRow;   // 顯示 "Material: <id>" / "Material: None"
    SharedPtr<Button> pBindBtn;       // 綁定第一個已載入貼圖到選取節點
    SharedPtr<Button> pUnbindBtn;     // 解綁(materialId = 0)
    String mMaterialLabel;            // 目前顯示文字

    void RebuildValueLabels();
    void RebuildMaterialLabel();
    void OnBindClicked();
    void OnUnbindClicked();
    static const char16_t *AxisLabel(InspectorAxis axis);
};

#endif // INSPECTOR_LAYER_HPP
