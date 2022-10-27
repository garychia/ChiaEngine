#include "ChiaApp.hpp"

int main()
{
    AppInfo info;
    info.appName = String("Chia Engine");
    ChiaApp app(info);
    return app.Execute();
}
