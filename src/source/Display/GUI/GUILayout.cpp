#include "Display/GUI/GUILayout.hpp"

void GUILayout::CalculateComponentDepths()
{
    float depthDistance = 0.95f / (pLayers.Length() * 2);
    float currentDepth = -0.95f;
    for (size_t i = 0; i < pLayers.Length(); i++)
    {
        const Point3D &pos = pLayers[i]->GetPosition();
        pLayers[i]->SetPosition(pos.x, pos.y, currentDepth);
        currentDepth += depthDistance;
        auto &pComponents = pLayers[i]->GetComponents();
        for (size_t j = 0; j < pComponents.Length(); j++)
        {
            const auto &componentPos = pComponents[j]->GetPosition();
            pComponents[j]->SetPosition(componentPos.x, componentPos.y, currentDepth);
        }
        currentDepth += depthDistance;
    }
}

GUILayout::GUILayout() : pLayers()
{
}

void GUILayout::AddLayer(SharedPtr<GUILayer> &pNewLayer)
{
    pLayers.Append(pNewLayer);
    CalculateComponentDepths();
}

void GUILayout::RefreshDepths()
{
    CalculateComponentDepths();
}

void GUILayout::SetWindowSize(const Point2D &newSize)
{
    for (size_t i = 0; i < pLayers.Length(); i++)
        pLayers[i]->SetWindowSize(newSize);
}

DynamicArray<SharedPtr<GUILayer>> &GUILayout::GetLayers()
{
    return pLayers;
}

const DynamicArray<SharedPtr<GUILayer>> &GUILayout::GetLayers() const
{
    return pLayers;
}

// ── 輸入派發(issue #64)────────────────────────────────────────────
// Hit-test 所有 layer 的 components;最後命中者(加入順序 = z 順序,較後
// 加入者較靠近 viewer)即最上層,優先取得事件。
SharedPtr<IInteractable> GUILayout::HitTest(const Point2D &coordinates)
{
    SharedPtr<IInteractable> pTopmost;
    for (size_t i = 0; i < pLayers.Length(); i++)
    {
        auto &pComponents = pLayers[i]->GetComponents();
        for (size_t j = 0; j < pComponents.Length(); j++)
        {
            IInteractable *pInteractable = dynamic_cast<IInteractable *>(pComponents[j].GetRaw());
            if (pInteractable && pInteractable->WithIn(coordinates))
                pTopmost = pComponents[j]; // SharedPtr<IGUI> → SharedPtr<IInteractable>(downcast copy)
        }
    }
    return pTopmost;
}

bool GUILayout::DispatchMouseDown(const Point2D &coordinates)
{
    pPressed = HitTest(coordinates);
    if (!pPressed)
        return false;
    pPressed->OnMouseDown(coordinates);
    return true;
}

bool GUILayout::DispatchMouseUp(const Point2D &coordinates)
{
    SharedPtr<IInteractable> pReleased = pPressed;
    pPressed = SharedPtr<IInteractable>();
    if (!pReleased)
        return false;
    // click 語意:按下與放開都在同一元件內才視為點擊
    // (Button::OnMouseUp / OnClicked 自帶 WithIn guard,雙重保險)
    if (pReleased->WithIn(coordinates))
        pReleased->OnClicked(coordinates);
    pReleased->OnMouseUp(coordinates);
    return true;
}

bool GUILayout::DispatchMouseMove(const Point2D &coordinates)
{
    SharedPtr<IInteractable> pTarget = HitTest(coordinates);
    if (!pTarget)
        return false;
    pTarget->OnHovered(coordinates);
    return true;
}
