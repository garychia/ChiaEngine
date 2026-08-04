#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "Data/DynamicArray.hpp"
#include "Scene.hpp"
#include "Display/Renderer.hpp"
#include "System/Input/KeyCombination.hpp"
#include "System/Input/MouseInfo.hpp"
#include "WindowInfo.hpp"
#include "pch.hpp"

class GUILayout;

class Window
{
  protected:
    WindowHandle handle;

    Window *pParent;

    WindowInfo info;

    Scene *pScene;

    GUILayout *pGUILayout; // 走 Frame 的 GUI 佈局(P6:Frame 是唯一渲染貨幣)

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

    // 設定本視窗要經由 Frame 命令流繪製的 GUI 佈局(取代 legacy renderer.Render(layout))。
    void SetGUILayout(GUILayout *pLayout);

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
