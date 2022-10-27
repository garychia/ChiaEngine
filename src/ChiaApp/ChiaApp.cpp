#include "ChiaApp.hpp"

#include "Display/WindowManager.hpp"

#define DEFAULT_MAIN_WINDOW_WIDTH 1000
#define DEFAULT_MAIN_WINDOW_HEIGHT 800

ChiaApp::ChiaApp(const AppInfo &info) : App(info), pMainWindow(nullptr), fullScreen(false)
{
}

bool ChiaApp::Initialize()
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

void ChiaApp::Finalize()
{
    UnregisterClass((LPCWSTR)info.appName.CStr(), GetModuleHandle(NULL));
}

bool ChiaApp::LoadData()
{
    return true;
}

int ChiaApp::Execute()
{
    if (!Initialize())
        return EXIT_FAILURE;
    WindowInfo winInfo(info.appName, String("Chia Engine"), false, DEFAULT_MAIN_WINDOW_WIDTH,
                       DEFAULT_MAIN_WINDOW_HEIGHT);
    pMainWindow = (Panel *)WindowManager::GetSingleton().ConstructWindow<Panel>(winInfo);
    if (!pMainWindow)
    {
        Finalize();
        return EXIT_FAILURE;
    }
    pMainWindow->Show();
    if (!LoadData())
    {
        Finalize();
        return EXIT_FAILURE;
    }

    MSG msg = {};
    bool shouldContinue = false;
    while (!shouldContinue)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Update();
        Render();
        shouldContinue = msg.message == WM_QUIT;
    }

    Finalize();
    return EXIT_SUCCESS;
}
