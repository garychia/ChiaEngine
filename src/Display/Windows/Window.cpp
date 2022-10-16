#include "Display/Window.hpp"

#include "Globals.hpp"
#include "pch.hpp"


Window::Window(const WindowInfo &info)
    : handle(NULL), pParent(nullptr), pInfo(info), pScene(nullptr), renderer(), pChildren()
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

bool Window::AddChild(Window *pChild)
{
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
    pInfo.fullScreen = pInfo.fullScreen && pParent;
    const auto windowWidth = pInfo.fullScreen ? GetSystemMetrics(SM_CXSCREEN) : pInfo.width;
    const auto windowHeight = pInfo.fullScreen ? GetSystemMetrics(SM_CYSCREEN) : pInfo.height;
    const int winPosX = pInfo.fullScreen ? 0 : pInfo.positionFromLeft;
    const int winPosY = pInfo.fullScreen ? 0 : pInfo.positionFromTop;

    if (pInfo.fullScreen)
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
        CreateWindowEx(WS_EX_APPWINDOW, (LPCWSTR)AppName.CStr(), (LPCWSTR)pInfo.title.CStr(),
                       !pParent ? WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN : WS_CHILDWINDOW | WS_VISIBLE, winPosX, winPosY,
                       windowWidth, windowHeight, pParent ? pParent->GetHandle() : NULL, NULL, AppInstance, NULL);
    if (!handle)
    {
        if (pInfo.fullScreen)
        {
            ChangeDisplaySettings(NULL, 0);
            FullScreen = false;
        }
        return false;
    }

    if (!renderer.Initialize(handle, pInfo.fullScreen))
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
    return pInfo;
}

const WindowInfo &Window::GetWindowInfo() const
{
    return pInfo;
}

void Window::SetPosition(unsigned long newX, unsigned long newY)
{
    pInfo.positionFromLeft = newX;
    pInfo.positionFromTop = newY;
    MoveWindow(GetHandle(), newX, newY, pInfo.width, pInfo.height, TRUE);
}

void Window::SetSize(unsigned long newWidth, unsigned long newHeight)
{
    pInfo.width = newWidth;
    pInfo.height = newHeight;
    MoveWindow(GetHandle(), pInfo.positionFromLeft, pInfo.positionFromTop, newWidth, newHeight, TRUE);
}

void Window::Destroy()
{
    for (size_t i = 0; i < pChildren.Length(); i++)
        pChildren[i]->Destroy();
    if (handle)
        DestroyWindow(handle);
    handle = nullptr;
}

void Window::OnCameraChanged(const Camera *pCamera)
{
    renderer.ApplyCamera(pCamera);
}

void Window::OnWindowResized(long newWidth, long newHeight)
{
    renderer.OnWindowResized(newWidth, newHeight);
}

bool Window::OnKeyboardInputReceived(const KeyCombination &keys)
{
    for (size_t i = 0; i < pChildren.Length(); i++)
    {
        if (pChildren[i]->OnKeyboardInputReceived(keys))
            return true;
    }
    return false;
}

bool Window::OnMouseInputReceived(const MouseInfo &mouseInfo)
{
    for (size_t i = 0; i < pChildren.Length(); i++)
    {
        if (pChildren[i]->OnMouseInputReceived(mouseInfo))
            return true;
    }
    return false;
}
