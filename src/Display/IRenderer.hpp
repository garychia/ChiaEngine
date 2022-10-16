#ifndef IRENDERER_HPP
#define IRENDERER_HPP

#include "Camera.hpp"
#include "Scene.hpp"
#include "Shader.hpp"
#include "pch.hpp"


class IRenderer
{
  public:
    virtual bool Initialize(WindowHandle windowHandle, bool fullScreen) = 0;

    virtual bool SwitchToFullScreen() = 0;

    virtual bool SwitchToWindowMode() = 0;

    virtual bool LoadScene(Scene &scene) = 0;

    virtual bool AddVertexShader(Shader *pShader) = 0;

    virtual bool AddPixelShader(Shader *pShader) = 0;

    virtual void ApplyCamera(const Camera *pCamera) = 0;

    virtual void OnCameraChanged() = 0;

    virtual void OnWindowResized(long newWidth, long newHeight) = 0;

    virtual void Update() = 0;

    virtual void Render(const Scene &scene) = 0;

    virtual void Clear() = 0;
};

#endif
