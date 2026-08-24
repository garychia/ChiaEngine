#ifndef GUI_LAYOUT_HPP
#define GUI_LAYOUT_HPP

#include "Data/DynamicArray.hpp"
#include "Data/Pointers.hpp"
#include "Display/GUI/IInteractable.hpp"
#include "GUILayer.hpp"

class GUILayout
{
  private:
    DynamicArray<SharedPtr<GUILayer>> pLayers;

    // 滑鼠按下的目標(click 語意:down + up 都在同一元件內才算點擊)
    SharedPtr<IInteractable> pPressed;

    void CalculateComponentDepths();

    // 回傳最上層命中互動元件:depth 由 CalculateComponentDepths 排定,
    // 後加入的 layer/component z 較高(較接近 viewer)→ 最後命中者即最上層。
    SharedPtr<IInteractable> HitTest(const Point2D &coordinates);

  public:
    GUILayout();

    void AddLayer(SharedPtr<GUILayer> &pNewLayer);

    // #67:CalculateComponentDepths 原本只在 AddLayer 裡跑,動態加 components
    // (editor BuildHierarchy 重build列)後 z 永遠停留 0 → 重疊元件深度錯。
    // 新增 public wrapper,佈局內容變動後重排深度。
    void RefreshDepths();

    void SetWindowSize(const Point2D &newSize);

    DynamicArray<SharedPtr<GUILayer>> &GetLayers();

    const DynamicArray<SharedPtr<GUILayer>> &GetLayers() const;

    // 輸入派發(issue #64,editor 前置):滑鼠事件 hit-test 佈局內所有
    // IInteractable 元件並觸發對應事件。回傳 true = 有人消費(事件不該
    // 繼續漏給場景);false = 未命中任何互動元件。
    bool DispatchMouseDown(const Point2D &coordinates);

    bool DispatchMouseUp(const Point2D &coordinates);

    bool DispatchMouseMove(const Point2D &coordinates);
};

#endif // GUI_LAYOUT_HPP
