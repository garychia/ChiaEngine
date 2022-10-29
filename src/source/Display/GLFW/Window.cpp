#include "Display/Window.hpp"

#include "System/Debug/Debug.hpp"
#include "pch.hpp"

Window::Window(const WindowInfo &info)
    : handle(NULL), pParent(nullptr), info(info), pScene(nullptr), renderer(), pChildren()
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
}

bool Window::Initialize(Window *pParent)
{
    this->pParent = pParent;
    info.fullScreen = info.fullScreen && pParent;
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
    char title[1024];
    info.title.ToUTF8(title, sizeof(title));
    handle = glfwCreateWindow(info.GetWidth(), info.GetHeight(), title,
                              info.fullScreen ? glfwGetPrimaryMonitor() : NULL, pParent ? pParent->GetHandle() : NULL);
    return !!handle;
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

void Window::SetPosition(unsigned long newX, unsigned long newY)
{
    info.border.xPos = newX;
    info.border.yPos = newY;
    glfwSetWindowPos(GetHandle(), newX, newY);
}

void Window::SetSize(unsigned long newWidth, unsigned long newHeight)
{
    info.border.width = newWidth;
    info.border.height = newHeight;
    glfwSetWindowSize(GetHandle(), newWidth, newHeight);
}

void Window::Destroy()
{
    for (size_t i = 0; i < pChildren.Length(); i++)
        pChildren[i]->Destroy();
    if (handle)
        glfwDestroyWindow(handle);
    handle = nullptr;
}

void Window::OnCameraChanged(WeakPtr<Camera> &pCamera)
{
    renderer.ApplyCamera(pCamera);
}

void Window::OnWindowResized(long newWidth, long newHeight)
{
    info.border.width = newWidth;
    info.border.height = newHeight;
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
