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
    WNDCLASSEX wndClass = {};
    wndClass.hInstance = GetModuleHandle(NULL);
    wndClass.lpfnWndProc = WindowManager::WndProc;
    wndClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wndClass.hIconSm = wndClass.hIcon;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.lpszClassName = (LPCWSTR)info.appName.CStr();
    wndClass.cbSize = sizeof(WNDCLASSEX);
    return RegisterClassEx(&wndClass);
}

void App::Finalize()
{
    UnregisterClass((LPCWSTR)info.appName.CStr(), GetModuleHandle(NULL));
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
