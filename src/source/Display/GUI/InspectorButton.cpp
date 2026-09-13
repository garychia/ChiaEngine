#include "Display/GUI/InspectorButton.hpp"
#include "Display/GUI/TransformEditCommand.hpp"
#include "Scene/TransformComponent.hpp"

void EditTransformComponent(SceneSystem *pScene, uint32_t entityIndex, InspectorAxis axis, float sign)
{
    // #82:軸→delta 的對應留在 View,實際寫入走 SceneSystem 的編輯器 seam
    // (EditTransform),不再直接存取 pScene->world 的元件。
    if (!pScene)
        return;
    // #82 fix:delta 的 scale 預設必須是 (0,0,0) — TransformComponent 預設
    // scale=(1,1,1) 是完整 TRS 值,不是增量;若不歸零,每個 edit 都會把
    // scale 加上 (1,1,1),污染 scale(實測:一次 SclY edit 後 scale 變 (5,4.1,5))。
    TransformComponent delta;
    delta.scale = Point3D(0, 0, 0);
    switch (axis)
    {
        case InspectorAxis::PositionX: delta.position.x = sign * 0.5f; break;
        case InspectorAxis::PositionY: delta.position.y = sign * 0.5f; break;
        case InspectorAxis::PositionZ: delta.position.z = sign * 0.5f; break;
        case InspectorAxis::RotationX: delta.rotation.x = sign * 5.0f; break;
        case InspectorAxis::RotationY: delta.rotation.y = sign * 5.0f; break;
        case InspectorAxis::RotationZ: delta.rotation.z = sign * 5.0f; break;
        case InspectorAxis::ScaleX: delta.scale.x = sign * 0.1f; break;
        case InspectorAxis::ScaleY: delta.scale.y = sign * 0.1f; break;
        case InspectorAxis::ScaleZ: delta.scale.z = sign * 0.1f; break;
    }
    pScene->EditTransform(entityIndex, delta);
}

InspectorButton::InspectorButton(const Point2D &windowSize, const Border &border, SceneSystem *pScene,
                                 UndoStack *pUndoStack, uint32_t targetEntityIndex, InspectorAxis axis, float sign)
    : Button(windowSize, border), pScene(pScene), pUndoStack(pUndoStack),
      targetEntityIndex(targetEntityIndex), axis(axis), sign(sign)
{
}

void InspectorButton::SetTarget(uint32_t entityIndex)
{
    targetEntityIndex = entityIndex;
}

void InspectorButton::OnClicked(const Point2D &coordinates)
{
    Button::OnClicked(coordinates);
    if (!WithIn(coordinates))
        return;
    // ADR-0001 D5:push undoable command (push applies), instead of direct edit.
    if (pUndoStack)
        pUndoStack->Push(new TransformEditCommand(pScene, targetEntityIndex, axis, sign));
    else
        EditTransformComponent(pScene, targetEntityIndex, axis, sign);
}
