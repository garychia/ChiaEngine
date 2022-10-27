#include "ChiaApp.hpp"

#include "Display/WindowManager.hpp"
#include "Globals.hpp"

ChiaApp::ChiaApp(const AppInfo &info) : App(info), pMainWindow(nullptr)
{
}

bool ChiaApp::Initialize()
{
    MainWindow = nullptr;
    FullScreen = false;
    AppInstance = GetModuleHandle(NULL);
    WNDCLASSEX wndClass = {};
    wndClass.hInstance = AppInstance;
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
    if (FullScreen)
    {
        ChangeDisplaySettings(NULL, 0);
        FullScreen = false;
    }
    if (AppInstance)
    {
        UnregisterClass((LPCWSTR)info.appName.CStr(), AppInstance);
        AppInstance = nullptr;
    }
}

bool ChiaApp::LoadData()
{
    return true;
}

int ChiaApp::Execute()
{
    if (!Initialize())
        return EXIT_FAILURE;
    WindowInfo winInfo(info.appName, String("Chia Engine"), false, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
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
