#include "GUILayout.hpp"

GUILayout::GUILayout() : pLayers()
{
}

void GUILayout::AddLayer(SharedPtr<GUILayer> &pNewLayer)
{
    pLayers.Append(pNewLayer);
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
