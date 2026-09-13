#include "Display/Scene.hpp"

Scene::Scene(SceneType type) : type(type), pRenderables(), onCameraChanged()
{
}

Scene::~Scene()
{
    pRenderables.RemoveAll();
    if (SharedPtr<Camera> strong = pCamera.Lock())
        strong->onChanged.Unsubscribe(this);
}

void Scene::AddRenderable(const SharedPtr<IRenderable> &pRenderable)
{
    pRenderables.Append(pRenderable);
}

void Scene::AddRenderables(const DynamicArray<SharedPtr<IRenderable>> &pRenderables)
{
    for (size_t i = 0; i < pRenderables.Length(); i++)
        AddRenderable(pRenderables[i]);
}

Scene::SceneType Scene::GetType() const
{
    return type;
}

DynamicArray<SharedPtr<IRenderable>> &Scene::GetRenderables()
{
    return pRenderables;
}

const DynamicArray<SharedPtr<IRenderable>> &Scene::GetRenderables() const
{
    return pRenderables;
}

void Scene::ApplyCamera(WeakPtr<Camera> pCamera)
{
    // #86:WeakPtr 不再可直接解引用 — 一律先 Lock()(目標已亡 → 空 SharedPtr,安全略過)。
    if (SharedPtr<Camera> strongOld = this->pCamera.Lock())
        strongOld->onChanged.Unsubscribe(this);
    this->pCamera = pCamera;
    if (SharedPtr<Camera> strongNew = pCamera.Lock())
        strongNew->onChanged.Subscribe(this, &Scene::OnCameraChanged);
    onCameraChanged.Invoke(pCamera);
}

void Scene::RemoveCamera()
{
    ApplyCamera(WeakPtr<Camera>());
}

WeakPtr<Camera> &Scene::GetCamera()
{
    return pCamera;
}

const WeakPtr<Camera> &Scene::GetCamera() const
{
    return pCamera;
}

void Scene::OnCameraChanged()
{
    onCameraChanged.Invoke(pCamera);
}
