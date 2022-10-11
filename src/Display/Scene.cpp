#include "Scene.hpp"

void Scene::OnCameraChanged(const SharedPtr<Camera> pCamera)
{
    onCameraChanged.Invoke(pCamera);
}

Scene::Scene(SceneType type) : type(type), pRenderables(), onCameraChanged()
{
}

Scene::~Scene()
{
    for (size_t i = 0; i < pRenderables.Length(); i++)
        delete pRenderables[i];
    if (pCamera.IsValid())
        pCamera->onChanged.Unsubscribe(this);
}

Scene::SceneType Scene::GetType() const
{
    return type;
}

DynamicArray<IRenderable *> &Scene::GetRenderables()
{
    return pRenderables;
}

const DynamicArray<IRenderable *> &Scene::GetRenderables() const
{
    return pRenderables;
}

void Scene::ApplyCamera(SharedPtr<Camera> &pCamera)
{
    if (this->pCamera == pCamera)
        return;
    if (this->pCamera.IsValid())
        this->pCamera->onChanged.Unsubscribe(this);
    this->pCamera = pCamera;
    if (pCamera.IsValid())
        pCamera->onChanged.Subscribe(this, &Scene::OnCameraChanged);
    onCameraChanged.Invoke(pCamera);
}

void Scene::RemoveCamera()
{
    pCamera.Release();
    onCameraChanged.Invoke(pCamera);
}

WeakPtr<Camera> &Scene::GetCamera()
{
    return pCamera;
}

const WeakPtr<Camera> &Scene::GetCamera() const
{
    return pCamera;
}
