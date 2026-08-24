#ifndef INSPECTOR_BUTTON_HPP
#define INSPECTOR_BUTTON_HPP

#include "Button.hpp"
#include "Scene/SceneSystem.hpp"

// Editor inspector 的 + / - 調整鈕(issue #68 step 2):綁定一個 target entity
// (以 index 辨識)與一個 axis(X/Y/Z of position/rotation/scale)以及方向(+/-)。
// 點擊時直接編輯該 entity 的 TransformComponent。Button 自身持有 SceneSystem*,
// 不依賴函數指標回呼(引擎風格:無捕獲 lambda)。
enum class InspectorAxis
{
    PositionX,
    PositionY,
    PositionZ,
    RotationX,
    RotationY,
    RotationZ,
    ScaleX,
    ScaleY,
    ScaleZ
};

// 共用編輯邏輯:對 entity 的 TransformComponent 給定 axis 加上 sign * step。
// step 依軸類型(position 0.5 / rotation 5.0 / scale 0.1)。
void EditTransformComponent(SceneSystem *pScene, uint32_t entityIndex, InspectorAxis axis, float sign);

class InspectorButton : public Button
{
  private:
    SceneSystem *pScene;
    uint32_t targetEntityIndex;
    InspectorAxis axis;
    float sign; // +1 / -1

  public:
    InspectorButton(const Point2D &windowSize, const Border &border, SceneSystem *pScene,
                    uint32_t targetEntityIndex, InspectorAxis axis, float sign);

    // 選取 entity 改變時更新 target(避免每次 SelectEntity 重建整批鈕)。
    void SetTarget(uint32_t entityIndex);

    virtual void OnClicked(const Point2D &coordinates) override;
};

#endif // INSPECTOR_BUTTON_HPP
