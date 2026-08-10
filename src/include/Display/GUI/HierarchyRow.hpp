#ifndef HIERARCHY_ROW_HPP
#define HIERARCHY_ROW_HPP

#include "Button.hpp"
#include "System/Operation/Event.hpp"
#include "System/World/Entity.hpp"

// Editor hierarchy 列(issue #60 step 1):Button 子類,綁定一個 SceneSystem 節點。
// 點擊經 #64 的 hit-test 派發觸發 OnClicked 覆寫,以 rowClicked 帶 entity 廣播 —
// Button::clickEvent 無參數,無法表達「哪一列被點」,故 Panel 用一個 handler
// 接所有列的 rowClicked。Inspector(step 2)消費選取的 entity。
class HierarchyRow : public Button
{
  private:
    Entity entity;

  public:
    Event<void(Entity)> rowClicked;

    HierarchyRow(const Point2D &windowSize, const Border &border, const Entity &entity);

    virtual void OnClicked(const Point2D &coordinates) override;

    Entity GetEntity() const;
};

#endif // HIERARCHY_ROW_HPP
