#ifndef INSPECTOR_LAYER_HPP
#define INSPECTOR_LAYER_HPP

#include "Display/GUI/GUILayer.hpp"
#include "Display/GUI/Button.hpp"
#include "Display/GUI/InspectorButton.hpp"
#include "Scene/SceneSystem.hpp"

// Editor inspector(#68 step 2):右側面板,顯示並編輯目前選取 entity 的
// TransformComponent(position / rotation / scale,各 3 軸)。每軸一行:
// 左側文字欄(目前值)+ 右側 + / - 兩顆調整鈕。
// 邏輯全在 Sim/View 純資料層,可無頭測試(SceneSystem + InspectorLayer)。
class InspectorLayer : public GUILayer
{
  private:
    static const float StepPosition;
    static const float StepRotation;
    static const float StepScale;
    static const float RowHeight;
    static const float ColLabelWidth;
    static const float BtnWidth;

    SceneSystem *pScene;
    uint32_t selectedEntityIndex;
    bool pendingRefresh;

    // 9 個 field row(只顯示文字):index 0..8 對應 InspectorAxis 順序。
    SharedPtr<Button> pFieldRows[9];
    // 9 × 2 個 + / - 鈕(同樣順序)。
    SharedPtr<InspectorButton> pButtons[18];

    void RebuildValueLabels();
    static const char16_t *AxisLabel(InspectorAxis axis);

  public:
    InspectorLayer(const Point2D &windowSize, const Border &border, SceneSystem *pScene);

    // 設定目前選取的 entity(由 Panel 在點擊 hierarchy 列時呼叫)。
    void SelectEntity(uint32_t entityIndex);

    // 每幀由 Panel::Render 呼叫:反映 selectedEntity / 外部 transform 改動。
    void Update();

    // 測試/按鈕共用:對 entity 的 TransformComponent 給定 axis 加上 sign * step。
    void ApplyEdit(uint32_t entityIndex, InspectorAxis axis, float sign);

    // 測試用:取 field row 陣列(只顯示文字的 9 顆)。
    SharedPtr<Button> *GetFieldRows();
};

#endif // INSPECTOR_LAYER_HPP
