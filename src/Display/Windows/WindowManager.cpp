#include "Display/WindowManager.hpp"
#include "System/Input/InputHandler.hpp"
#include "System/Input/KeyboardHandler.hpp"
#include "System/Input/MouseInput.hpp"
#include "System/Input/Windows/WinKeyCode.hpp"
#include "pch.hpp"

LRESULT CALLBACK WindowManager::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        return 0;
    case WM_CLOSE: {
        PostQuitMessage(0);
        return 0;
    }
    case WM_SIZE: {
        RECT rect;
        GetClientRect(hwnd, &rect);
        WindowManager::GetSingleton().HandleResizing(hwnd, rect.right, rect.bottom);
        return 0;
    }
    }
    return WndInputHandler(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WindowManager::WndInputHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KILLFOCUS:
        KeyboardHandler::GetSingleton().OnWindowLoseFocus();
        MouseInput::GetSingleton().OnWindowLoseFocus();
        ReleaseCapture();
        return 0;
    case WM_LBUTTONDOWN: {
        SetCapture(hwnd);
        const auto xPos = GET_X_LPARAM(lParam);
        const auto yPos = GET_Y_LPARAM(lParam);
        MouseInput &mouseInput = MouseInput::GetSingleton();
        mouseInput.OnMouseLeftButtonDown(xPos, yPos);
        if (WindowManager::GetSingleton().HandleMouseInput(hwnd, mouseInput.GetMouseInfo()))
            return 0;
        break;
    }
    case WM_MOUSEMOVE: {
        const auto xPos = GET_X_LPARAM(lParam);
        const auto yPos = GET_Y_LPARAM(lParam);
        MouseInput &mouseInput = MouseInput::GetSingleton();
        mouseInput.OnMouseMove(xPos, yPos);
        if (WindowManager::GetSingleton().HandleMouseInput(hwnd, mouseInput.GetMouseInfo()))
            return 0;
        break;
    }
    case WM_LBUTTONUP: {
        ReleaseCapture();
        const auto xPos = GET_X_LPARAM(lParam);
        const auto yPos = GET_Y_LPARAM(lParam);
        MouseInput &mouseInput = MouseInput::GetSingleton();
        mouseInput.OnMouseLeftButtonUp(xPos, yPos);
        if (WindowManager::GetSingleton().HandleMouseInput(hwnd, mouseInput.GetMouseInfo()))
            return 0;
        break;
    }
    case WM_MOUSEWHEEL: {
        const auto distance = GET_WHEEL_DELTA_WPARAM(wParam);
        MouseInput &mouseInput = MouseInput::GetSingleton();
        mouseInput.OnWheelRotated(distance);
        if (WindowManager::GetSingleton().HandleMouseInput(hwnd, mouseInput.GetMouseInfo()))
            return 0;
        break;
    }
    case WM_SYSKEYDOWN:
        break;
    case WM_SYSKEYUP:
        break;
    case WM_SYSCHAR:
        break;
    case WM_KEYDOWN: {
        KeyCode keyCode = ConvertToKeyCode(wParam);
        auto &keyHandler = KeyboardHandler::GetSingleton();
        keyHandler.ProcessKeyDown(keyCode);
        if (keyCode == KeyCode::KeyCodeUndefined ||
            WindowManager::GetSingleton().HandleKeyInput(hwnd, keyHandler.GetKeyCombination()))
            break;
        return 0;
    }
    case WM_KEYUP: {
        KeyCode keyCode = ConvertToKeyCode(wParam);
        auto &keyHandler = KeyboardHandler::GetSingleton();
        keyHandler.ProcessKeyUp(keyCode);
        if (keyCode == KeyCode::KeyCodeUndefined ||
            WindowManager::GetSingleton().HandleKeyInput(hwnd, keyHandler.GetKeyCombination()))
            break;
        return 0;
    }
    case WM_CHAR: {
        KeyboardHandler::GetSingleton().RecordChar(wParam);
        return 0;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

WindowManager::WindowManager() : windowMap(), pWindows()
{
}

WindowManager::~WindowManager()
{
    for (size_t i = 0; i < pWindows.Length(); i++)
        delete pWindows[i];
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
