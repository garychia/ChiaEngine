#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "IRenderer.hpp"
#include "Display/GUI/GUILayout.hpp"

#ifdef DIRECTX_ENABLED
#include "Display/DirectX/DirectXRenderer.hpp"
#endif

class Camera;

class Renderer : public IRenderer
{
  private:
#ifdef DIRECTX_ENABLED
    DirectXRenderer specializedRenderer;
#endif
  public:
    Renderer();

    virtual bool Initialize(const Window *pWindow) override;

    virtual bool LoadScene(Scene &scene) override;

    virtual bool LoadGUILayout(GUILayout &layout) override;

    virtual bool AddVertexShader(Shader &shader) override;

    virtual bool AddPixelShader(Shader &shader) override;

    virtual bool SwitchToFullScreen() override;

    virtual bool SwitchToWindowMode() override;

    virtual void ApplyCamera(WeakPtr<Camera> pCamera) override;

    virtual void OnCameraChanged() override;

    virtual void OnWindowResized(long newWidth, long newHeight) override;

    virtual void Update() override;

    virtual void Render(Scene &scene) override;

    virtual void Render(GUILayout &layout) override;

    virtual void Clear() override;
};

#endif
