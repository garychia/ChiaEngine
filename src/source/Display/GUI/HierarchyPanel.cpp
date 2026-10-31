#include "Display/GUI/HierarchyPanel.hpp"
#include "Display/GUI/EditorSession.hpp"
#include "Display/GUI/InspectorLayer.hpp"

const float HierarchyPanel::RowHeight = 20.f;
const float HierarchyPanel::IndentPerDepth = 14.f;

HierarchyPanel::HierarchyPanel(const String &title, const Point2D &windowSize, const Border &border,
                               PanelDock dock, SceneSystem *pScene, EditorSession *pSession,
                               InspectorLayer *pInspector)
    : PanelPane(title, windowSize, border, dock), mpScene(pScene), mpSession(pSession),
      mpInspector(pInspector), mRows()
{
    SetColor(Color(0.08f, 0.08f, 0.10f));
    Rebuild();
}

HierarchyPanel::~HierarchyPanel()
{
    mRows.RemoveAll();
}

void HierarchyPanel::Rebuild()
{
    mRows.RemoveAll();
    if (!mpScene)
        return;

    // ADR-0001 D3:依 SceneSystem seam 讀取節點樹(roots 依 index、children 依 index)。
    uint32_t rowIndex = 0;
    DynamicArray<Entity> roots = mpScene->GetRoots();
    for (size_t i = 0; i < roots.GetNElements(); i++)
        BuildSubtree(roots[i], 0, rowIndex);
}

void HierarchyPanel::BuildSubtree(Entity e, uint32_t depth, uint32_t &rowIndex)
{
    if (!mpScene->Alive(e))
        return;

    // 節點名稱與 index 顯示於行標籤。
    String name;
    mpScene->GetNodeName(e.GetIndex(), name);

    const float x = IndentPerDepth * static_cast<float>(depth);
    const float y = static_cast<float>(rowIndex) * RowHeight;
    const float width = 200.f;
    const float height = RowHeight - 2.f;

    auto pRow = AddComponent<HierarchyRow>(Point2D(), Border(x, y, width, height), e);
    pRow->SetLabel(name);
    pRow->SetFontSize(12.f);
    pRow->rowClicked.Subscribe(this, &HierarchyPanel::OnRowClicked);
    mRows.Append(pRow);

    rowIndex++;

    // 遞迴子孫(依 index 升序,確定性)。
    DynamicArray<Entity> children = mpScene->GetChildren(e);
    for (size_t i = 0; i < children.GetNElements(); i++)
        BuildSubtree(children[i], depth + 1, rowIndex);
}

void HierarchyPanel::Update()
{
    // 目前無動態高亮(選取列反色)需求,保留預留擴充點。
    (void)0;
}

void HierarchyPanel::OnRowClicked(Entity entity)
{
    if (!mpScene || !mpSession)
        return;

    if (!mpScene->Alive(entity))
        return;

    mpSession->Select(entity.GetIndex());

    // 刷新 Inspector 面板(選取變更立即反映)。
    if (mpInspector)
        mpInspector->SetSelection(&mpSession->GetSelection());
}
