#include "Display/GUI/InspectorButton.hpp"
#include "Scene/TransformComponent.hpp"

void EditTransformComponent(SceneSystem *pScene, uint32_t entityIndex, InspectorAxis axis, float sign)
{
    if (!pScene)
        return;
    Entity e = pScene->world.GetEntityByIndex(entityIndex);
    TransformComponent *pT = pScene->world.GetComponent<TransformComponent>(e);
    if (!pT)
        return;
    float step = 0.f;
    float *pVal = nullptr;
    switch (axis)
    {
        case InspectorAxis::PositionX: step = 0.5f; pVal = &pT->position.x; break;
        case InspectorAxis::PositionY: step = 0.5f; pVal = &pT->position.y; break;
        case InspectorAxis::PositionZ: step = 0.5f; pVal = &pT->position.z; break;
        case InspectorAxis::RotationX: step = 5.0f; pVal = &pT->rotation.x; break;
        case InspectorAxis::RotationY: step = 5.0f; pVal = &pT->rotation.y; break;
        case InspectorAxis::RotationZ: step = 5.0f; pVal = &pT->rotation.z; break;
        case InspectorAxis::ScaleX: step = 0.1f; pVal = &pT->scale.x; break;
        case InspectorAxis::ScaleY: step = 0.1f; pVal = &pT->scale.y; break;
        case InspectorAxis::ScaleZ: step = 0.1f; pVal = &pT->scale.z; break;
    }
    if (pVal)
        *pVal += sign * step;
}

InspectorButton::InspectorButton(const Point2D &windowSize, const Border &border, SceneSystem *pScene,
                                 uint32_t targetEntityIndex, InspectorAxis axis, float sign)
    : Button(windowSize, border), pScene(pScene), targetEntityIndex(targetEntityIndex), axis(axis), sign(sign)
{
}

void InspectorButton::SetTarget(uint32_t entityIndex)
{
    targetEntityIndex = entityIndex;
}

void InspectorButton::OnClicked(const Point2D &coordinates)
{
    Button::OnClicked(coordinates);
    if (WithIn(coordinates))
        EditTransformComponent(pScene, targetEntityIndex, axis, sign);
}
