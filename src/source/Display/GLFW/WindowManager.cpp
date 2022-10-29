#include "Display/WindowManager.hpp"

#include "Display/WindowManager.hpp"
#include "System/Input/InputHandler.hpp"
#include "System/Input/KeyboardHandler.hpp"
#include "System/Input/MouseInput.hpp"
#include "System/Input/Windows/WinKeyCode.hpp"
#include "pch.hpp"

WindowManager::WindowManager() : windowMap(), pWindows()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

WindowManager::~WindowManager()
{
    for (size_t i = 0; i < pWindows.Length(); i++)
        delete pWindows[i];
    glfwTerminate();
}

WindowManager &WindowManager::GetSingleton()
{
    static WindowManager singleton;
    return singleton;
}

void WindowManager::UpdateWindows()
{
    for (size_t i = 0; i < pWindows.Length(); i++)
        pWindows[i]->Update();
}

void WindowManager::RenderWindows()
{
    for (size_t i = 0; i < pWindows.Length(); i++)
        pWindows[i]->Render();
}

void WindowManager::HandleResizing(const WindowHandle &handle, long newWidth, long newHeight)
{
    if (!windowMap.Contains(handle))
        return;
    Window *pWindow = windowMap[handle];
    pWindow->OnWindowResized(newWidth, newHeight);
}

bool WindowManager::HandleKeyInput(const WindowHandle &handle, const KeyCombination &keys)
{
    Window *pWindow = windowMap[handle];
    return pWindow->OnKeyboardInputReceived(keys);
}

bool WindowManager::HandleMouseInput(const WindowHandle &handle, const MouseInfo &mouseInfo)
{
    Window *pWindow = windowMap[handle];
    return pWindow->OnMouseInputReceived(mouseInfo);
}
