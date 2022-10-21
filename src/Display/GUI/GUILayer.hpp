#ifndef GUI_LAYER_HPP
#define GUI_LAYER_HPP

#include "Data/DynamicArray.hpp"
#include "Data/Pointers.hpp"
#include "IGUI.hpp"

class GUILayer : public IGUI
{
  protected:
    DynamicArray<SharedPtr<IGUI>> pComponents;

  public:
    GUILayer(const Point2D &windowSize, const Border &border);

    template <class GUIType, class... Args> SharedPtr<IGUI> AddComponent(Args... args)
    {
        auto pComponent = SharedPtr<IGUI>::Construct<GUIType>(args...);
        pComponents.Append(pComponent);
        return pComponent;
    }

    void AddComponent(SharedPtr<IGUI> &pComponent);

    void RemoveComponents();

    DynamicArray<SharedPtr<IGUI>> &GetComponents();

    const DynamicArray<SharedPtr<IGUI>> &GetComponents() const;

    DynamicArray<SharedPtr<IRenderable>> GetRenderables() const;

    virtual void SetWindowSize(const Point2D &newSize) override;
};

#endif // GUI_LAYER_HPP
