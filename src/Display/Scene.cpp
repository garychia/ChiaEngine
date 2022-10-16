#include "Scene.hpp"

Scene::Scene(SceneType type) : type(type), pRenderables(), onCameraChanged()
{
}

Scene::~Scene()
{
    pRenderables.RemoveAll();
    if (pCamera.IsValid())
        pCamera->onChanged.Unsubscribe(this);
}

void Scene::AddRenderable(const SharedPtr<IRenderable> &pRenderable)
{
    pRenderables.Append(pRenderable);
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

void Scene::ApplyCamera(SharedPtr<Camera> &pCamera)
{
    if (!pCamera.IsValid() || this->pCamera == pCamera)
        return;
    if (this->pCamera.IsValid())
        this->pCamera->onChanged.Unsubscribe(this);
    this->pCamera = pCamera;
    if (pCamera.IsValid())
        pCamera->onChanged.Subscribe(this, &Scene::OnCameraChanged);
    onCameraChanged.Invoke(pCamera.GetRaw());
}

void Scene::RemoveCamera()
{
    pCamera.Release();
    onCameraChanged.Invoke(pCamera.GetRaw());
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
    onCameraChanged.Invoke(pCamera.GetRaw());
}
