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
    enum SceneType
    {
        Game,
        GUI
    };

    Event<void(const Camera *)> onCameraChanged;

    Scene(SceneType type = Game);

    ~Scene();

    void AddRenderable(const SharedPtr<IRenderable> &pRenderable);

    SceneType GetType() const;

    DynamicArray<SharedPtr<IRenderable>> &GetRenderables();

    const DynamicArray<SharedPtr<IRenderable>> &GetRenderables() const;

    void ApplyCamera(SharedPtr<Camera> &pCamera);

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
