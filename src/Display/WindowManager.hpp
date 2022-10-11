#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include "Data/DynamicArray.hpp"
#include "Data/HashTable.hpp"
#include "Window.hpp"


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

    Window *ConstructWindow(const WindowInfo &info);

    Window *ConstructChildWindow(Window *pParent, const WindowInfo &childInfo);

    void UpdateWindows();

    void RenderWindows();

    void HandleResizing(const WindowHandle &handle);

    bool HandleKeyInput(const WindowHandle &handle, const KeyCombination &keys);

    bool HandleMouseInput(const WindowHandle &handle, const MouseInfo &mouseInfo);
};

#endif // WINDOW_MANAGER_HPP
