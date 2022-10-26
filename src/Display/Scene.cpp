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
    if (this->pCamera.IsValid())
        this->pCamera->onChanged.Unsubscribe(this);
    this->pCamera = pCamera;
    if (pCamera.IsValid())
        pCamera->onChanged.Subscribe(this, &Scene::OnCameraChanged);
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
