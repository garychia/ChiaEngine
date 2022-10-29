#include "ChiaApp.hpp"

int main()
{
    AppInfo info;
    info.appName = String("Chia Engine");
    ChiaApp app(info);
    app.Initialize();
    auto result = app.Execute();
    app.Finalize();
    return result;
}
