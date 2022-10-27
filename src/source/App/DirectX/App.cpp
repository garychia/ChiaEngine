#include "App/App.hpp"

#include "Display/WindowManager.hpp"

void App::Update()
{
    WindowManager::GetSingleton().UpdateWindows();
}

void App::Render()
{
    WindowManager::GetSingleton().RenderWindows();
}

App::App(const AppInfo &info) : info(info)
{
}

App::~App()
{
}

int App::Execute()
{
    if (!Initialize())
        return EXIT_FAILURE;
    Finalize();
    return EXIT_SUCCESS;
}
