#include "App/App.hpp"

#include "Globals.hpp"
#include "Display/WindowManager.hpp"
#include "System/Debug/Debug.hpp"

bool App::Initialize()
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
    wndClass.lpszClassName = (LPCWSTR)AppName.CStr();
    wndClass.cbSize = sizeof(WNDCLASSEX);
    return RegisterClassEx(&wndClass);
}

void App::Finalize()
{
    if (FullScreen)
    {
        ChangeDisplaySettings(NULL, 0);
        FullScreen = false;
    }
    if (AppInstance)
    {
        UnregisterClass((LPCWSTR)AppName.CStr(), AppInstance);
        AppInstance = nullptr;
    }
}

bool App::LoadData()
{
    return true;
}

void App::Update()
{
    WindowManager::GetSingleton().UpdateWindows();
}

void App::Render()
{
    WindowManager::GetSingleton().RenderWindows();
}

App::App() : pMainWindow(nullptr)
{
}

App::~App()
{
}

int App::Execute()
{
    if (!Initialize())
        return EXIT_FAILURE;
    WindowInfo winInfo(String("Chia Engine"), false, WINDOW_WIDTH, WINDOW_HEIGHT);
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
