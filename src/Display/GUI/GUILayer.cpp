#include "GUILayer.hpp"

GUILayer::GUILayer(const Point2D &windowSize, const Border &border) : IGUI(windowSize, border), pComponents()
{
}

GUILayer::~GUILayer()
{
}

void GUILayer::AddComponent(SharedPtr<IGUI> &pComponent)
{
    pComponents.Append(pComponent);
}

void GUILayer::RemoveComponents()
{
    pComponents.RemoveAll();
}

DynamicArray<SharedPtr<IGUI>> &GUILayer::GetComponents()
{
    return pComponents;
}

const DynamicArray<SharedPtr<IGUI>> &GUILayer::GetComponents() const
{
    return pComponents;
}

DynamicArray<SharedPtr<IRenderable>> GUILayer::GetRenderables() const
{
    DynamicArray<SharedPtr<IRenderable>> pItems;
    pItems.Append(GetRenderable());
    for (size_t i = 0; i < pComponents.Length(); i++)
        pItems.Append(pComponents[i]->GetRenderable());
    return pItems;
}

void GUILayer::SetWindowSize(const Point2D &newSize)
{
    IGUI::SetWindowSize(newSize);
    for (size_t i = 0; i < pComponents.Length(); i++)
        pComponents[i]->SetWindowSize(newSize);
}
