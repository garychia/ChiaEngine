#include "PanelLayout.hpp"

#include "Data/Str.hpp"

const unsigned long PanelLayout::TopBarHeight = 30;

const unsigned long PanelLayout::SidebarWidth = 180;

const unsigned long PanelLayout::RowHeight = 22;

PanelLayout::PanelLayout(const Point2D &windowSize) : GUILayout(), pHierarchyLayer(), pHierarchyRows()
{
    auto pTopPanelBar = SharedPtr<GUILayer>::Construct<TopPanelBar>(windowSize);
    AddLayer(pTopPanelBar);

    // 左側 hierarchy 側欄:toolbar 下方一整條,背景深灰與場景區分。
    pHierarchyLayer =
        SharedPtr<GUILayer>::Construct(windowSize, Border(0.f, TopBarHeight, SidebarWidth, 400.f));
    pHierarchyLayer->SetColor(Color(0.13f, 0.13f, 0.15f));
    AddLayer(pHierarchyLayer);
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
}

DynamicArray<SharedPtr<HierarchyRow>> &PanelLayout::GetHierarchyRows()
{
    return pHierarchyRows;
}
