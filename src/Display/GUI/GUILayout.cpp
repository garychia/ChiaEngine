#include "GUILayout.hpp"

GUILayout::GUILayout() : pLayers()
{
}

void GUILayout::AddLayer(SharedPtr<GUILayer> &pNewLayer)
{
    pLayers.Append(pNewLayer);
}

DynamicArray<SharedPtr<IRenderable>> GUILayout::GetRenderables() const
{
    DynamicArray<SharedPtr<IRenderable>> pItems;
    for (size_t i = 0; i < pLayers.Length(); i++)
    {
        auto pChildItems = pLayers[i]->GetRenderables();
        for (size_t j = 0; j < pChildItems.Length(); j++)
            pItems.Append(pChildItems[j]);
    }
    return pItems;
}
