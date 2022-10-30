#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "Data/DynamicArray.hpp"
#include "Scene.hpp"
#include "Display/Renderer.hpp"
#include "System/Input/KeyCombination.hpp"
#include "System/Input/MouseInfo.hpp"
#include "WindowInfo.hpp"
#include "pch.hpp"

class Window
{
  protected:
    WindowHandle handle;

    Window *pParent;

    WindowInfo info;

    Scene *pScene;

    Renderer renderer;

    DynamicArray<Window *> pChildren;

    Window(const WindowInfo &info);

    DynamicArray<Window *> &GetChildren();

    const DynamicArray<Window *> &GetChildren() const;

    bool AddChild(Window *pChild);

  public:
    ~Window();

    virtual bool Initialize(Window *pParent = nullptr);

    virtual bool LoadScene(Scene &scene);

    bool Show();

    virtual void Update();

    virtual void Render();

    WindowHandle GetHandle() const;

    Window *GetParent() const;

    WindowInfo &GetWindowInfo();

    const WindowInfo &GetWindowInfo() const;

    virtual void SetPosition(unsigned long newX, unsigned long newY);

    virtual void SetSize(unsigned long newWidth, unsigned long newHeight);

    virtual void Destroy();

    virtual void OnCameraChanged(WeakPtr<Camera> &pCamera);

    virtual void OnWindowResized(long newWidth, long newHeight);

    virtual bool OnKeyboardInputReceived(const KeyCombination &keys);

    virtual bool OnMouseInputReceived(const MouseInfo &mouseInfo);

    friend class WindowManager;
};

#endif // WINDOW_HPP
