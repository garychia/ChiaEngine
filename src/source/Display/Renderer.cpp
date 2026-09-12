#include "Display/Renderer.hpp"
#include "Display/Camera.hpp"

Renderer::Renderer() : specializedRenderer()
{
}

bool Renderer::Initialize(const Window *pWindow)
{
    return specializedRenderer.Initialize(pWindow);
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

bool Renderer::Execute(const Frame &frame)
{
#ifdef VULKAN_ENABLED
    return specializedRenderer.Execute(frame);
#else
    // DirectX/OpenGL 是 legacy backend,未實作 Frame/IFrameExecutor 契約 →
    // Execute 不可用(見 docs/agents/directx-backend-assessment.md)。
    (void)frame;
    return false;
#endif
}
