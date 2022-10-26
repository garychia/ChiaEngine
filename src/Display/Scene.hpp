#ifndef SCENE_HPP
#define SCENE_HPP

#include "Camera.hpp"
#include "Data/DynamicArray.hpp"
#include "Data/Pointers.hpp"
#include "IRenderable.hpp"
#include "System/Operation/Event.hpp"

class Scene
{
  public:
    enum class SceneType
    {
        Game,
        GUI
    };

    Event<void(WeakPtr<Camera> &)> onCameraChanged;

    Scene(SceneType type = SceneType::Game);

    ~Scene();

    void AddRenderable(const SharedPtr<IRenderable> &pRenderable);

    void AddRenderables(const DynamicArray<SharedPtr<IRenderable>> &pRenderables);

    SceneType GetType() const;

    DynamicArray<SharedPtr<IRenderable>> &GetRenderables();

    const DynamicArray<SharedPtr<IRenderable>> &GetRenderables() const;

    void ApplyCamera(WeakPtr<Camera> pCamera);

    void RemoveCamera();

    WeakPtr<Camera> &GetCamera();

    const WeakPtr<Camera> &GetCamera() const;

  private:
    DynamicArray<SharedPtr<IRenderable>> pRenderables;

    WeakPtr<Camera> pCamera;

    Scene::SceneType type;

    void OnCameraChanged();
};

#endif
