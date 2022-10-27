#include "ChiaApp.hpp"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    AppInfo info;
    info.appName = String("Chia Engine");
    ChiaApp app(info);
    return app.Execute();
}
