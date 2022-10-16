#include "Renderer.hpp"
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

bool Renderer::AddVertexShader(Shader *pShader)
{
    return specializedRenderer.AddVertexShader(pShader);
}

bool Renderer::AddPixelShader(Shader *pShader)
{
    return specializedRenderer.AddPixelShader(pShader);
}

bool Renderer::SwitchToFullScreen()
{
    return specializedRenderer.SwitchToFullScreen();
}

bool Renderer::SwitchToWindowMode()
{
    return specializedRenderer.SwitchToWindowMode();
}

void Renderer::ApplyCamera(const Camera *pCamera)
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

void Renderer::Render(const Scene &scene)
{
    specializedRenderer.Render(scene);
}

void Renderer::Clear()
{
    specializedRenderer.Clear();
}
