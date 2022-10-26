#ifndef IRENDERER_HPP
#define IRENDERER_HPP

#include "Camera.hpp"
#include "Scene.hpp"
#include "Shader.hpp"
#include "Display/GUI/GUILayout.hpp"
#include "pch.hpp"


class IRenderer
{
  public:
    virtual bool Initialize(WindowHandle windowHandle, bool fullScreen) = 0;

    virtual bool SwitchToFullScreen() = 0;

    virtual bool SwitchToWindowMode() = 0;

    virtual bool LoadScene(Scene &scene) = 0;

    virtual bool LoadGUILayout(GUILayout &layout) = 0;

    virtual bool AddVertexShader(Shader &shader) = 0;

    virtual bool AddPixelShader(Shader &shader) = 0;

    virtual void ApplyCamera(WeakPtr<Camera> pCamera) = 0;

    virtual void OnCameraChanged() = 0;

    virtual void OnWindowResized(long newWidth, long newHeight) = 0;

    virtual void Update() = 0;

    virtual void Render(Scene &scene) = 0;

    virtual void Render(GUILayout &layout) = 0;

    virtual void Clear() = 0;
};

#endif
