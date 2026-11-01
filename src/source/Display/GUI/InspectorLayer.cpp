#include "Display/GUI/InspectorLayer.hpp"
#include "Display/GUI/EditorSession.hpp" // GetTextureCount/GetTexturePathAt/BindTextureToSelectedEntity
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

InspectorLayer::InspectorLayer(const String &title, const Point2D &windowSize, const Border &border,
                               PanelDock dock, SceneSystem *pScene, UndoStack *pUndoStack,
                               EditorSession *pSession)
    : PanelPane(title, windowSize, border, dock), pScene(pScene), pUndoStack(pUndoStack),
      pSelection(nullptr), mpSession(pSession), pMaterialRow(), pBindBtn(), pUnbindBtn()
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

        // - / + 鈕(初始 target = 0;SetSelection 會以 Selection.entityIndex 刷新)。
        auto pMinus = AddComponent<InspectorButton>(windowSize,
                                                     Border(ColLabelWidth + 6.f, y, BtnWidth, RowHeight - 2.f),
                                                     pScene, pUndoStack, 0, axis, -1.f);
        pMinus->SetLabel(String(u"-"));
        pMinus->SetFontSize(12.f);
        auto pPlus = AddComponent<InspectorButton>(windowSize,
                                                   Border(ColLabelWidth + 6.f + BtnWidth, y, BtnWidth, RowHeight - 2.f),
                                                   pScene, pUndoStack, 0, axis, +1.f);
        pPlus->SetLabel(String(u"+"));
        pPlus->SetFontSize(12.f);
        pButtons[i * 2] = pMinus;
        pButtons[i * 2 + 1] = pPlus;
    }

    // ---- 材質區(9 個 transform 行之後)----
    const float matY = 9.f * RowHeight + 4.f;

    // 材質標籤顯示行(只讀文字)。
    auto pRow = AddComponent<Button>(windowSize, Border(4.f, matY, ColLabelWidth, RowHeight - 2.f));
    pRow->SetColor(Color(0.10f, 0.10f, 0.12f));
    pRow->SetFontSize(12.f);
    pRow->SetTextColor(Color(0.9f, 0.9f, 0.92f));
    pMaterialRow = pRow;

    // Bind 鈕:綁定第一個已載入貼圖到選取節點。
    auto pBind = AddComponent<Button>(windowSize,
                                      Border(ColLabelWidth + 6.f, matY, BtnWidth, RowHeight - 2.f));
    pBind->SetLabel(String(u"Bind"));
    pBind->SetFontSize(10.f);
    pBind->pressEvent.Subscribe(this, &InspectorLayer::OnBindClicked);
    pBindBtn = pBind;

    // Unbind 鈕:materialId = 0。
    auto pUnbind = AddComponent<Button>(windowSize,
                                        Border(ColLabelWidth + 6.f + BtnWidth, matY, BtnWidth, RowHeight - 2.f));
    pUnbind->SetLabel(String(u"Unbind"));
    pUnbind->SetFontSize(10.f);
    pUnbind->pressEvent.Subscribe(this, &InspectorLayer::OnUnbindClicked);
    pUnbindBtn = pUnbind;

    RebuildValueLabels();
    RebuildMaterialLabel();
}

SharedPtr<Button> *InspectorLayer::GetFieldRows()
{
    return pFieldRows;
}

void InspectorLayer::SetSelection(const Selection *pNewSelection)
{
    pSelection = pNewSelection;
    for (size_t i = 0; i < 9; i++)
    {
        pButtons[i * 2]->SetTarget(pSelection->entityIndex);
        pButtons[i * 2 + 1]->SetTarget(pSelection->entityIndex);
    }
    RebuildValueLabels(); // 選取變更立即刷新(非每幀 churn)
    RebuildMaterialLabel();
}

void InspectorLayer::Update()
{
    RebuildValueLabels();
    RebuildMaterialLabel();
}

void InspectorLayer::ApplyEdit(uint32_t entityIndex, InspectorAxis axis, float sign)
{
    EditTransformComponent(pScene, entityIndex, axis, sign);
}

void InspectorLayer::RebuildValueLabels()
{
    if (!pScene || !pSelection || !pSelection->hasSelection)
    {
        // ADR-0001 D2:無選取 → 清空欄位(不顯示舊值、不崩)。
        for (size_t i = 0; i < 9; i++)
            pFieldRows[i]->SetLabel(String());
        return;
    }
    const uint32_t idx = pSelection->entityIndex;
    // #82:經 SceneSystem seam 讀取(存活 + 元件檢查在 Sim 內),不直接碰 world。
    TransformComponent tc;
    if (!pScene->GetTransform(idx, tc))
        return;
    const float vals[9] = {tc.position.x, tc.position.y, tc.position.z, tc.rotation.x, tc.rotation.y,
                           tc.rotation.z, tc.scale.x, tc.scale.y, tc.scale.z};
    for (size_t i = 0; i < 9; i++)
    {
        String label = String(AxisLabel(static_cast<InspectorAxis>(i)));
        label = label + String(u": ");
        label = label + String(Str<char16_t>::FromFloat(vals[i], 2));
        pFieldRows[i]->SetLabel(label);
    }
}

void InspectorLayer::RebuildMaterialLabel()
{
    if (!pScene || !mpSession || !pSelection || !pSelection->hasSelection)
    {
        mMaterialLabel = String(u"Material: None");
        pMaterialRow->SetLabel(mMaterialLabel);
        pBindBtn->SetLabel(String(u"Bind"));
        pUnbindBtn->SetLabel(String(u"Unbind"));
        return;
    }

    const uint64_t matId = mpSession->GetSelectedEntityMaterialId();
    if (matId == 0)
        mMaterialLabel = String(u"Material: None");
    else
    {
        String label = String(u"Material: ");
        label = label + String(Str<char16_t>::FromInt(matId));
        mMaterialLabel = label;
    }
    pMaterialRow->SetLabel(mMaterialLabel);
}

void InspectorLayer::OnBindClicked()
{
    if (!mpSession || !pSelection || !pSelection->hasSelection)
        return;
    if (mpSession->GetTextureCount() == 0)
        return; // 尚無已載入貼圖可綁定
    const String &firstPath = mpSession->GetTexturePathAt(0);
    mpSession->BindTextureToSelectedEntity(firstPath);
    RebuildMaterialLabel();
}

void InspectorLayer::OnUnbindClicked()
{
    if (!mpSession || !pSelection || !pSelection->hasSelection)
        return;
    mpSession->BindTextureToSelectedEntity(String());
    RebuildMaterialLabel();
}
