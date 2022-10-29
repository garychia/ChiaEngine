#include "ChiaApp.hpp"

#include "Display/WindowManager.hpp"

#define DEFAULT_MAIN_WINDOW_WIDTH 1000
#define DEFAULT_MAIN_WINDOW_HEIGHT 800

ChiaApp::ChiaApp(const AppInfo &info) : App(info)
{
}

int ChiaApp::Execute()
{
    WindowInfo winInfo(info.appName, String("Chia Engine"), false, DEFAULT_MAIN_WINDOW_WIDTH,
                       DEFAULT_MAIN_WINDOW_HEIGHT);
    pMainWindow = WindowManager::GetSingleton().ConstructWindow<Panel>(winInfo);
    if (!pMainWindow)
        return EXIT_FAILURE;
    if (!pMainWindow->Show())
        return EXIT_FAILURE;
    return App::Execute();
}
