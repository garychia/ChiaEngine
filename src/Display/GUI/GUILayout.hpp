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

    DynamicArray<SharedPtr<IRenderable>> GetRenderables() const;
};

#endif // GUI_LAYOUT_HPP
