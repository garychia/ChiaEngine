#include "Display/Renderer.hpp"
#include "Display/Camera.hpp"

Renderer::Renderer() : specializedRenderer()
{
}

bool Renderer::Initialize(WindowHandle windowHandle, bool fullScreen)
{
    return specializedRenderer.Initialize(windowHandle, fullScreen);
}

bool Renderer::LoadScene(Scene &scene)
{
    return specializedRenderer.LoadScene(scene);
}

bool Renderer::LoadGUILayout(GUILayout &layout)
{
    return specializedRenderer.LoadGUILayout(layout);
}

bool Renderer::AddVertexShader(Shader &shader)
{
    return specializedRenderer.AddVertexShader(shader);
}

bool Renderer::AddPixelShader(Shader &shader)
{
    return specializedRenderer.AddPixelShader(shader);
}

bool Renderer::SwitchToFullScreen()
{
    return specializedRenderer.SwitchToFullScreen();
}

bool Renderer::SwitchToWindowMode()
{
    return specializedRenderer.SwitchToWindowMode();
}

void Renderer::ApplyCamera(WeakPtr<Camera> pCamera)
{
    specializedRenderer.ApplyCamera(pCamera);
}

void Renderer::OnCameraChanged()
{
    specializedRenderer.OnCameraChanged();
}

void Renderer::OnWindowResized(long newWidth, long newHeight)
{
    specializedRenderer.OnWindowResized(newWidth, newHeight);
}

void Renderer::Update()
{
    specializedRenderer.Update();
}

void Renderer::Render(Scene &scene)
{
    specializedRenderer.Render(scene);
}

void Renderer::Render(GUILayout &layout)
{
    specializedRenderer.Render(layout);
}

void Renderer::Clear()
{
    specializedRenderer.Clear();
}
