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
            windowMap[pWindow->GetHandle()] = pWindow;
            pWindows.Append(pWindow);
        }
        else
        {
            delete pWindow;
            pWindow = nullptr;
        }
        return pWindow;
    }

    template <class WindowType, class ...Args> Window *ConstructChildWindow(Window *pParent, Args ...args)
    {
        Window *pChild = new WindowType(args...);
        if (!pChild->Initialize(pParent))
        {
            PRINTLN_ERR("Window: failed to add the child window.");
            delete pChild;
            return false;
        }
        if (!pParent->AddChild(pChild))
            return nullptr;
        windowMap[pChild->GetHandle()] = pChild;
        return pChild;
    }

    void UpdateWindows();

    void RenderWindows();

    void HandleResizing(const WindowHandle &handle, long newWidth, long newHeight);

    bool HandleKeyInput(const WindowHandle &handle, const KeyCombination &keys);

    bool HandleMouseInput(const WindowHandle &handle, const MouseInfo &mouseInfo);
};

#endif // WINDOW_MANAGER_HPP
