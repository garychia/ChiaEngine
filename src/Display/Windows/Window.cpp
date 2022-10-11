#include "Display/Window.hpp"

#include "pch.hpp"

extern bool FullScreen;
extern String AppName;
extern HINSTANCE AppInstance;

Window::Window(const WindowInfo &info)
    : handle(NULL), pParent(nullptr), info(info), pScene(nullptr), renderer(), pChildren(), inputLayer(),
      keyboardInputCallback(), mouseInputCallback()
{
}

DynamicArray<Window *> &Window::GetChildren()
{
    return pChildren;
}

const DynamicArray<Window *> &Window::GetChildren() const
{
    return pChildren;
}

bool Window::AddChild(const WindowInfo &childInfo)
{
    Window *pChild = new Window(childInfo);
    if (!pChild->Initialize(this))
    {
        PRINTLN_ERR("Window: failed to add the child window.");
        delete pChild;
        return false;
    }
    pChildren.Append(pChild);
    return true;
}

Window::~Window()
{
    Destroy();
    for (size_t i = 0; i < pChildren.Length(); i++)
        delete pChildren[i];
}

bool Window::Initialize(Window *pParent)
{
    this->pParent = pParent;
    info.fullScreen = info.fullScreen && pParent;
    const auto windowWidth = info.fullScreen ? GetSystemMetrics(SM_CXSCREEN) : info.width;
    const auto windowHeight = info.fullScreen ? GetSystemMetrics(SM_CYSCREEN) : info.height;
    const int winPosX = info.fullScreen ? 0 : info.positionFromLeft;
    const int winPosY = info.fullScreen ? 0 : info.positionFromTop;

    if (info.fullScreen)
    {
        FullScreen = true;

        DEVMODE devMode = {};
        devMode.dmPelsWidth = (DWORD)windowWidth;
        devMode.dmPelsHeight = (DWORD)windowHeight;
        devMode.dmBitsPerPel = 32;
        devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
        devMode.dmSize = sizeof(devMode);
        ChangeDisplaySettings(&devMode, CDS_FULLSCREEN);
    }

    handle =
        CreateWindowEx(WS_EX_APPWINDOW, (LPCWSTR)AppName.CStr(), (LPCWSTR)info.title.CStr(),
                       !pParent ? WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN : WS_CHILDWINDOW | WS_VISIBLE, winPosX, winPosY,
                       windowWidth, windowHeight, pParent ? pParent->GetHandle() : NULL, NULL, AppInstance, NULL);
    if (!handle)
    {
        if (info.fullScreen)
        {
            ChangeDisplaySettings(NULL, 0);
            FullScreen = false;
        }
        return false;
    }

    if (!renderer.Initialize(handle, info.fullScreen))
    {
        PRINTLN_ERR("Window: failed to initialize the renderer.");
        Destroy();
        return false;
    }
    return true;
}

bool Window::LoadScene(Scene &scene)
{
    this->pScene = &scene;
    scene.onCameraChanged.Subscribe(this, &Window::OnCameraChanged);
    return renderer.LoadScene(scene);
}

bool Window::Show()
{
    if (!ShowWindow(handle, SW_SHOW))
    {
        PRINTLN_ERR("Winodw: failed to display.");
        return false;
    }
    SetForegroundWindow(handle);
    SetFocus(handle);
    return true;
}

void Window::Update()
{
    for (size_t i = 0; i < pChildren.Length(); i++)
        pChildren[i]->Update();
    renderer.Update();
}

void Window::Render()
{
    for (size_t i = 0; i < pChildren.Length(); i++)
        pChildren[i]->Render();
    if (pScene)
        renderer.Render(*pScene);
    else
        renderer.Clear();
}

WindowHandle Window::GetHandle() const
{
    return handle;
}

Window *Window::GetParent() const
{
    return pParent;
}

WindowInfo &Window::GetWindowInfo()
{
    return info;
}

const WindowInfo &Window::GetWindowInfo() const
{
    return info;
}

InputHandleLayer &Window::GetInputHandleLayer()
{
    return inputLayer;
}

void Window::Destroy()
{
    for (size_t i = 0; i < pChildren.Length(); i++)
        pChildren[i]->Destroy();
    if (handle)
        DestroyWindow(handle);
    handle = nullptr;
}

void Window::OnCameraChanged(const SharedPtr<Camera> pCamera)
{
    renderer.ApplyCamera(pCamera.GetRaw());
}

void Window::OnWindowResized()
{
    renderer.OnWindowResized();
}

bool Window::OnKeyboardInputReceived(const KeyCombination &keys)
{
    if (keyboardInputCallback.Valid() && keyboardInputCallback(keys))
        return true;
    for (size_t i = 0; i < pChildren.Length(); i++)
    {
        if (pChildren[i]->OnKeyboardInputReceived(keys))
            return true;
    }
    return false;
}

bool Window::OnMouseInputReceived(const MouseInfo &mouseInfo)
{
    if (mouseInputCallback.Valid() && mouseInputCallback(mouseInfo))
        return true;
    for (size_t i = 0; i < pChildren.Length(); i++)
    {
        if (pChildren[i]->OnMouseInputReceived(mouseInfo))
            return true;
    }
    return false;
}
