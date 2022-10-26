#ifndef GUI_LAYOUT_HPP
#define GUI_LAYOUT_HPP

#include "Data/DynamicArray.hpp"
#include "Data/Pointers.hpp"
#include "GUILayer.hpp"

class GUILayout
{
  private:
    DynamicArray<SharedPtr<GUILayer>> pLayers;

  public:
    GUILayout();

    void AddLayer(SharedPtr<GUILayer> &pNewLayer);

    void SetWindowSize(const Point2D &newSize);

    DynamicArray<SharedPtr<GUILayer>> &GetLayers();

    const DynamicArray<SharedPtr<GUILayer>> &GetLayers() const;
};

#endif // GUI_LAYOUT_HPP
