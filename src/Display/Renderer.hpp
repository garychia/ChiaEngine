#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "IRenderer.hpp"

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

    virtual bool Initialize(WindowHandle windowHandle, bool fullScreen) override;

    virtual bool LoadScene(Scene &scene) override;

    virtual bool AddVertexShader(Shader *pShader) override;

    virtual bool AddPixelShader(Shader *pShader) override;

    virtual bool SwitchToFullScreen() override;

    virtual bool SwitchToWindowMode() override;

    virtual void ApplyCamera(const Camera *pCamera) override;

    virtual void OnCameraChanged() override;

    virtual void OnWindowResized() override;

    virtual void Update() override;

    virtual void Render(const Scene &scene) override;

    virtual void Clear() override;
};

#endif
