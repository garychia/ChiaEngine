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

    Event<void(const SharedPtr<Camera>)> onCameraChanged;

    Scene(SceneType type = Game);

    ~Scene();

    template <class T, class... Args> void AddRenderable(Args... args)
    {
        pRenderables.Append(new T(args...));
    }

    SceneType GetType() const;

    DynamicArray<IRenderable *> &GetRenderables();

    const DynamicArray<IRenderable *> &GetRenderables() const;

    void ApplyCamera(SharedPtr<Camera> &pCamera);

    void RemoveCamera();

    WeakPtr<Camera> &GetCamera();

    const WeakPtr<Camera> &GetCamera() const;

  private:
    DynamicArray<IRenderable *> pRenderables;

    WeakPtr<Camera> pCamera;

    Scene::SceneType type;

    void OnCameraChanged(const SharedPtr<Camera> pCamera);
};

#endif
