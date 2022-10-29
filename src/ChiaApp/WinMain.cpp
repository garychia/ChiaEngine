#include "ChiaApp.hpp"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    AppInfo info;
    info.appName = String("Chia Engine");
    info.appVersion.major = 1;
    info.appVersion.minor = 0;
    info.appVersion.patch = 0;
    info.engineVersion.major = 1;
    info.engineVersion.minor = 0;
    info.engineVersion.patch = 0;
    ChiaApp app(info);
    app.Initialize();
    auto result = app.Execute();
    app.Finalize();
    return result;
}
