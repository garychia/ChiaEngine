#include "App/App.hpp"

#include "Display/WindowManager.hpp"

App::App(const AppInfo &info) : info(info), mainLoop(), pMainWindow(nullptr)
{
}

App::~App()
{
}

bool App::Initialize()
{
#ifdef DIRECTX_ENABLED
    WNDCLASSEX wndClass = {};
    wndClass.hInstance = GetModuleHandle(NULL);
    wndClass.lpfnWndProc = WindowManager::WndProc;
    wndClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wndClass.hIconSm = wndClass.hIcon;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.lpszClassName = (LPCWSTR)info.appName.CStr();
    wndClass.cbSize = sizeof(WNDCLASSEX);
    return RegisterClassEx(&wndClass);
#else
    // GLFW backends (Vulkan / OpenGL):視窗類別由 GLFW 管理,無需註冊
    return true;
#endif
}

void App::Finalize()
{
#ifdef DIRECTX_ENABLED
    UnregisterClass((LPCWSTR)info.appName.CStr(), GetModuleHandle(NULL));
#endif
}

int App::Execute()
{
    while (mainLoop.ShouldContinue() && pMainWindow)
    {
        mainLoop.Execute(pMainWindow->GetHandle());
        Update();
        Render();
    }
    return EXIT_SUCCESS;
}

void App::Update()
{
    WindowManager::GetSingleton().UpdateWindows();
}

void App::Render()
{
    WindowManager::GetSingleton().RenderWindows();
}
