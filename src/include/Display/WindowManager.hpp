#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include "Data/DynamicArray.hpp"
#include "Data/HashTable.hpp"
#include "Window.hpp"
#include "System/Debug/Debug.hpp"

class WindowManager
{
  private:
    HashTable<WindowHandle, Window *> windowMap;

    DynamicArray<Window *> pWindows;

    WindowManager();

  public:
#ifdef DIRECTX_ENABLED
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static LRESULT CALLBACK WndInputHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
#endif

    ~WindowManager();

    static WindowManager &GetSingleton();

    template <class WindowType, class ...Args> Window *ConstructWindow(Args ...args)
    {
        Window *pWindow = new WindowType(args...);
        if (pWindow->Initialize())
        {
            RegisterWindow(pWindow);
            pWindows.Append(pWindow);
        }
        else
        {
            delete pWindow;
            pWindow = nullptr;
        }
        return pWindow;
    }

    // 以「現行 OS handle」把 window 註冊進查找表(輸入/調整大小 dispatch 用)。
    // 注意:GLFW 的 handle 在 Show() 才產生 — Construct*Window 階段 GetHandle()
    // 還是 NULL,因此 GLFW 必須在 Show() 建立 handle 後再呼叫一次(見 Window::Show)。
    void RegisterWindow(Window *pWindow)
    {
        windowMap[pWindow->GetHandle()] = pWindow;
    }

    template <class WindowType, class ...Args> Window *ConstructChildWindow(Window *pParent, Args ...args)
    {
        Window *pChild = new WindowType(args...);
        if (!pChild->Initialize(pParent))
        {
            PRINTLN_ERR("Window: failed to add the child window.");
            delete pChild;
            return nullptr;
        }
        if (!pParent->AddChild(pChild))
            return nullptr;
        RegisterWindow(pChild);
        return pChild;
    }

    void UpdateWindows();

    void RenderWindows();

    void HandleResizing(const WindowHandle &handle, long newWidth, long newHeight);

    bool HandleKeyInput(const WindowHandle &handle, const KeyCombination &keys);

    bool HandleMouseInput(const WindowHandle &handle, const MouseInfo &mouseInfo);
};

#endif // WINDOW_MANAGER_HPP
