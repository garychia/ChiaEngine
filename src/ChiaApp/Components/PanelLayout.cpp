#include "PanelLayout.hpp"

#include "Data/Str.hpp"

const unsigned long PanelLayout::TopBarHeight = PanelLayoutConstants::TopBarHeight;

const unsigned long PanelLayout::SidebarWidth = PanelLayoutConstants::SidebarWidth;

const unsigned long PanelLayout::RowHeight = PanelLayoutConstants::RowHeight;

const unsigned long PanelLayout::InspectorWidth = PanelLayoutConstants::InspectorWidth;

// ADR-0001 D5-resize:headless-testable region math 已移到 PanelRegions.hpp。

PanelLayout::PanelLayout(const Point2D &windowSize) : GUILayout(), pHierarchyLayer(), pHierarchyRows()
{
    auto pTopPanelBar = SharedPtr<GUILayer>::Construct<TopPanelBar>(windowSize);
    AddLayer(pTopPanelBar);

    // 左側 hierarchy 側欄:toolbar 下方一整條,背景深灰與場景區分。
    pHierarchyLayer =
        SharedPtr<GUILayer>::Construct(windowSize, Border(0.f, TopBarHeight, SidebarWidth, 400.f));
    pHierarchyLayer->SetColor(Color(0.13f, 0.13f, 0.15f));
    AddLayer(pHierarchyLayer);

    // 右側 inspector 由 CreateInspector 建立(InspectorLayer 自帶背景色),
    // SetRegions 會 reposition 它到 rightDock 位置。
}

void PanelLayout::BuildHierarchy(SceneSystem &scene)
{
    pHierarchyLayer->RemoveComponents();
    pHierarchyRows.RemoveAll();

    DynamicArray<Entity> nodes;
    DynamicArray<uint32_t> depths;
    scene.GetHierarchy(nodes, depths);

    const Point2D windowSize = pHierarchyLayer->GetWindowSize();
    const float indentStep = 12.f;
    for (size_t i = 0; i < nodes.GetNElements(); i++)
    {
        const float x = indentStep * depths[i] + 4.f;
        const float y = TopBarHeight + static_cast<float>(i) * RowHeight;
        auto pRow = pHierarchyLayer->AddComponent<HierarchyRow>(
            windowSize, Border(x, y, SidebarWidth - 8.f, RowHeight - 2.f), nodes[i]);
        pRow->SetColor(Color(0.22f, 0.22f, 0.25f));
        pRow->SetLabel(String(u"Entity ") + Str<char16_t>::FromInt(nodes[i].GetIndex()));
        pRow->SetFontSize(12.f);
        pRow->SetTextColor(Color(0.9f, 0.9f, 0.92f));
        pHierarchyRows.Append(pRow);
    }

    // #67:rows 在 AddLayer 之後才加入 → z 全 0。重排深度才能維持重疊順序。
    RefreshDepths();
}

void PanelLayout::SetRegions(const Point2D &windowSize, const PanelRegions &regions)
{
    SetWindowSize(windowSize);

    // 各 dock layer 用 regions 重新定位(border 直接改 x/y/w/h)。
    if (GetLayers().GetNElements() > 0)
    {
        Border &topBar = GetLayers()[0]->GetBorder();
        topBar.xPos = regions.topBar.xPos;
        topBar.yPos = regions.topBar.yPos;
        topBar.width = regions.topBar.width;
        topBar.height = regions.topBar.height;
        GetLayers()[0]->SetWindowSize(windowSize);
    }
    pHierarchyLayer->GetBorder().xPos = regions.leftDock.xPos;
    pHierarchyLayer->GetBorder().yPos = regions.leftDock.yPos;
    pHierarchyLayer->GetBorder().width = regions.leftDock.width;
    pHierarchyLayer->GetBorder().height = regions.leftDock.height;
    pHierarchyLayer->SetWindowSize(windowSize);

    if (pInspector)
    {
        Border &insp = pInspector->GetBorder();
        insp.xPos = regions.rightDock.xPos;
        insp.yPos = regions.rightDock.yPos;
        insp.width = regions.rightDock.width;
        insp.height = regions.rightDock.height;
        pInspector->SetWindowSize(windowSize);
    }

    // #67:動態改 panel 後 z 順序會亂,重排深度。
    RefreshDepths();
}

void PanelLayout::CreateInspector(SceneSystem &scene, UndoStack *pUndoStack)
{
    const Point2D windowSize = GetLayers().GetNElements() > 0 ? GetLayers()[0]->GetWindowSize() : Point2D(1000, 800);
    const float x = windowSize.x - InspectorWidth;
    pInspector = SharedPtr<InspectorLayer>::Construct<InspectorLayer>(
        windowSize, Border(x, PanelLayout::TopBarHeight, InspectorWidth, 400.f), &scene, pUndoStack);
    SharedPtr<GUILayer> pLayer = pInspector;
    AddLayer(pLayer);
}

InspectorLayer *PanelLayout::GetInspector()
{
    return pInspector.GetRaw();
}

DynamicArray<SharedPtr<HierarchyRow>> &PanelLayout::GetHierarchyRows()
{
    return pHierarchyRows;
}
