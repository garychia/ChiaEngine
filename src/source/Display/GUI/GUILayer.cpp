#include "Display/GUI/GUILayer.hpp"

GUILayer::GUILayer(const Point2D &windowSize, const Border &border) : IGUI(windowSize, border), pComponents()
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

void GUILayer::SetWindowSize(const Point2D &newSize)
{
    IGUI::SetWindowSize(newSize);
    for (size_t i = 0; i < pComponents.Length(); i++)
        pComponents[i]->SetWindowSize(newSize);
}
