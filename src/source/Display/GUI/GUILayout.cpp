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
