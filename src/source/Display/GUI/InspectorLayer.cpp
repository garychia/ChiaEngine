#include "Display/GUI/InspectorLayer.hpp"
#include "Display/GUI/InspectorButton.hpp"

const float InspectorLayer::StepPosition = 0.5f;
const float InspectorLayer::StepRotation = 5.0f;
const float InspectorLayer::StepScale = 0.1f;
const float InspectorLayer::RowHeight = 22.f;
const float InspectorLayer::ColLabelWidth = 140.f;
const float InspectorLayer::BtnWidth = 22.f;

const char16_t *InspectorLayer::AxisLabel(InspectorAxis axis)
{
    switch (axis)
    {
        case InspectorAxis::PositionX: return u"Pos X";
        case InspectorAxis::PositionY: return u"Pos Y";
        case InspectorAxis::PositionZ: return u"Pos Z";
        case InspectorAxis::RotationX: return u"Rot X";
        case InspectorAxis::RotationY: return u"Rot Y";
        case InspectorAxis::RotationZ: return u"Rot Z";
        case InspectorAxis::ScaleX: return u"Scl X";
        case InspectorAxis::ScaleY: return u"Scl Y";
        case InspectorAxis::ScaleZ: return u"Scl Z";
    }
    return u"?";
}

InspectorLayer::InspectorLayer(const Point2D &windowSize, const Border &border, SceneSystem *pScene)
    : GUILayer(windowSize, border), pScene(pScene), selectedEntityIndex(0), pendingRefresh(true)
{
    SetColor(Color(0.10f, 0.10f, 0.12f));
    for (size_t i = 0; i < 9; i++)
    {
        const float y = static_cast<float>(i) * RowHeight + 4.f;
        const InspectorAxis axis = static_cast<InspectorAxis>(i);

        // 文字欄:顯示 axis 名 + 目前值。
        auto pRow = AddComponent<Button>(windowSize, Border(4.f, y, ColLabelWidth, RowHeight - 2.f));
        pRow->SetColor(Color(0.10f, 0.10f, 0.12f));
        pRow->SetFontSize(12.f);
        pRow->SetTextColor(Color(0.9f, 0.9f, 0.92f));
        pFieldRows[i] = pRow;

        // - / + 鈕(初始 target = 0,SelectEntity 會刷新)。
        auto pMinus = AddComponent<InspectorButton>(windowSize,
                                                     Border(ColLabelWidth + 6.f, y, BtnWidth, RowHeight - 2.f), pScene,
                                                     selectedEntityIndex, axis, -1.f);
        pMinus->SetLabel(String(u"-"));
        pMinus->SetFontSize(12.f);
        auto pPlus = AddComponent<InspectorButton>(windowSize,
                                                   Border(ColLabelWidth + 6.f + BtnWidth, y, BtnWidth, RowHeight - 2.f),
                                                   pScene, selectedEntityIndex, axis, +1.f);
        pPlus->SetLabel(String(u"+"));
        pPlus->SetFontSize(12.f);
        pButtons[i * 2] = pMinus;
        pButtons[i * 2 + 1] = pPlus;
    }
    RebuildValueLabels();
}

SharedPtr<Button> *InspectorLayer::GetFieldRows()
{
    return pFieldRows;
}

void InspectorLayer::SelectEntity(uint32_t entityIndex)
{
    selectedEntityIndex = entityIndex;
    for (size_t i = 0; i < 9; i++)
    {
        pButtons[i * 2]->SetTarget(entityIndex);
        pButtons[i * 2 + 1]->SetTarget(entityIndex);
    }
    pendingRefresh = true;
    RebuildValueLabels();
}

void InspectorLayer::Update()
{
    if (pScene && pendingRefresh)
    {
        RebuildValueLabels();
        pendingRefresh = false;
    }
}

void InspectorLayer::ApplyEdit(uint32_t entityIndex, InspectorAxis axis, float sign)
{
    EditTransformComponent(pScene, entityIndex, axis, sign);
    pendingRefresh = true;
}

void InspectorLayer::RebuildValueLabels()
{
    if (!pScene)
        return;
    Entity e = pScene->world.GetEntityByIndex(selectedEntityIndex);
    if (!pScene->world.Alive(e))
        return;
    TransformComponent *pT = pScene->world.GetComponent<TransformComponent>(e);
    if (!pT)
        return;
    const float vals[9] = {pT->position.x, pT->position.y, pT->position.z, pT->rotation.x, pT->rotation.y,
                            pT->rotation.z, pT->scale.x, pT->scale.y, pT->scale.z};
    for (size_t i = 0; i < 9; i++)
    {
        String label = String(AxisLabel(static_cast<InspectorAxis>(i)));
        label = label + String(u": ");
        label = label + String(Str<char16_t>::FromFloat(vals[i], 2));
        pFieldRows[i]->SetLabel(label);
    }
}
