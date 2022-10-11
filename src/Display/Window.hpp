#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "Data/DynamicArray.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include "System/Input/InputHandleLayer.hpp"
#include "System/Operation/Event.hpp"
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
    InputHandleLayer inputLayer;

    Window(const WindowInfo &info);

    DynamicArray<Window *> &GetChildren();

    const DynamicArray<Window *> &GetChildren() const;

    bool AddChild(const WindowInfo &childInfo);

  public:
    Callback<bool(const KeyCombination &)> keyboardInputCallback;

    Callback<bool(const MouseInfo &)> mouseInputCallback;

    ~Window();

    bool Initialize(Window *pParent = nullptr);

    bool LoadScene(Scene &scene);

    bool Show();

    void Update();

    void Render();

    WindowHandle GetHandle() const;

    Window *GetParent() const;

    WindowInfo &GetWindowInfo();

    const WindowInfo &GetWindowInfo() const;

    InputHandleLayer &GetInputHandleLayer();

    void Destroy();

    void OnCameraChanged(const SharedPtr<Camera> pCamera);

    void OnWindowResized();

    bool OnKeyboardInputReceived(const KeyCombination &keys);

    bool OnMouseInputReceived(const MouseInfo &mouseInfo);

    friend class WindowManager;
};

#endif // WINDOW_HPP
