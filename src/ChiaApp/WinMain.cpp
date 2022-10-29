#include "ChiaApp.hpp"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    AppInfo info;
    info.appName = String("Chia Engine");
    ChiaApp app(info);
    app.Initialize();
    auto result = app.Execute();
    app.Finalize();
    return result;
}
