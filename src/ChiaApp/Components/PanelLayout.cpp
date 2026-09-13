#include "PanelLayout.hpp"

#include "Data/Str.hpp"
#include "Display/GUI/EditorSession.hpp"

const unsigned long PanelLayout::TopBarHeight = PanelLayoutConstants::TopBarHeight;

const unsigned long PanelLayout::SidebarWidth = PanelLayoutConstants::SidebarWidth;

const unsigned long PanelLayout::RowHeight = PanelLayoutConstants::RowHeight;

const unsigned long PanelLayout::InspectorWidth = PanelLayoutConstants::InspectorWidth;

// ADR-0001 D5-resize:headless-testable region math 已移到 PanelRegions.hpp。

PanelLayout::PanelLayout(const Point2D &windowSize, EditorSession &session)
    : GUILayout(), mSession(session), mWindowSize(windowSize), pHierarchyRows()
{
    // 頂部工具列 = TopBar dock 的 named pane(HorizontalList 子類,高度 30)。
    // 舊版用 GetLayers()[0] 魔術 index 在 SetRegions 裡特判;現在 registry
    // 依 dock 統一處理。
    RegisterPane(SharedPtr<PanelPane>::Construct<TopPanelBar>(windowSize, PanelLayout::TopBarHeight));
}

PanelPane *PanelLayout::RegisterPane(SharedPtr<PanelPane> pPane)
{
    if (!pPane)
        return nullptr;
    if (PanelPane *pExisting = mSession.GetPane(pPane->GetTitle()))
        return pExisting;
    PanelPane *pRaw = mSession.RegisterPane(pPane);
    SharedPtr<GUILayer> pLayer = pPane; // SharedPtr<PanelPane> → SharedPtr<GUILayer>
    AddLayer(pLayer);
    return pRaw;
}

void PanelLayout::BuildHierarchy(SceneSystem &scene)
{
    PanelPane *pHierarchy = RegisterPane(SharedPtr<PanelPane>::Construct(
        String(u"Hierarchy"), mWindowSize, Border(0.f, TopBarHeight, SidebarWidth, 400.f), PanelDock::LeftDock));
    pHierarchy->RemoveComponents();
    pHierarchyRows.RemoveAll();

    DynamicArray<Entity> nodes;
    DynamicArray<uint32_t> depths;
    scene.GetHierarchy(nodes, depths);

    const float indentStep = 12.f;
    for (size_t i = 0; i < nodes.GetNElements(); i++)
    {
        const float x = indentStep * depths[i] + 4.f;
        const float y = TopBarHeight + static_cast<float>(i) * RowHeight;
        auto pRow = pHierarchy->AddComponent<HierarchyRow>(
            mWindowSize, Border(x, y, SidebarWidth - 8.f, RowHeight - 2.f), nodes[i]);
        pRow->SetColor(Color(0.22f, 0.22f, 0.25f));
        pRow->SetLabel(String(u"Entity ") + Str<char16_t>::FromInt(nodes[i].GetIndex()));
        pRow->SetFontSize(12.f);
        pRow->SetTextColor(Color(0.9f, 0.9f, 0.92f));
        pHierarchyRows.Append(pRow);
    }

    // #67:rows 在 AddLayer 之後才加入 → z 全 0。重排深度才能維持重疊順序。
    RefreshDepths();
}

void PanelLayout::CreateInspector(SceneSystem &scene)
{
    const float x = mWindowSize.x - InspectorWidth;
    RegisterPane(SharedPtr<PanelPane>::Construct<InspectorLayer>(
        String(u"Inspector"), mWindowSize, Border(x, TopBarHeight, InspectorWidth, 400.f), PanelDock::RightDock,
        &scene, &mSession.GetUndoStack()));
}

void PanelLayout::SetRegions(const Point2D &windowSize, const PanelRegions &regions)
{
    mWindowSize = windowSize;
    SetWindowSize(windowSize);

    // ADR-0001 D3:不再特判 GetLayers()[0] / 成員 slot — 走 session 的 pane
    // registry,依各自 dock assignment 定位。collapsed pane 高度歸零。
    const DynamicArray<SharedPtr<PanelPane>> &panes = mSession.GetPanes();
    for (size_t i = 0; i < panes.GetNElements(); i++)
    {
        SharedPtr<PanelPane> pPaneCopy = panes[i];
        PanelPane *pPane = pPaneCopy.GetRaw();
        const Border region = RegionForDock(pPane->GetDock(), regions);
        if (pPane->IsCollapsed())
            pPane->GetBorder() = Border(region.xPos, region.yPos, region.width, 0.f);
        else
            pPane->GetBorder() = region;
        pPane->SetWindowSize(windowSize);
    }

    // #67:動態改 panel 後 z 順序會亂,重排深度。
    RefreshDepths();
}

Border PanelLayout::RegionForDock(PanelDock dock, const PanelRegions &regions)
{
    switch (dock)
    {
        case PanelDock::TopBar: return regions.topBar;
        case PanelDock::LeftDock: return regions.leftDock;
        case PanelDock::RightDock: return regions.rightDock;
        case PanelDock::CenterViewport: return regions.centerViewport;
    }
    return Border();
}

InspectorLayer *PanelLayout::GetInspector()
{
    return dynamic_cast<InspectorLayer *>(mSession.GetPane(String(u"Inspector")));
}

DynamicArray<SharedPtr<HierarchyRow>> &PanelLayout::GetHierarchyRows()
{
    return pHierarchyRows;
}