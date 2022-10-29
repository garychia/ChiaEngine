#include "App/App.hpp"

#include "Display/WindowManager.hpp"

App::App(const AppInfo &info) : info(info), mainLoop(), pMainWindow(nullptr)
{
}

App::~App()
{
}

bool App::Initialize()
{
    return true;
}

void App::Finalize()
{
}

int App::Execute()
{
    while (mainLoop.ShouldContinue() && pMainWindow)
    {
        mainLoop.Execute(pMainWindow->GetHandle());
        Update();
        Render();
    }
    return EXIT_SUCCESS;
}

void App::Update()
{
    WindowManager::GetSingleton().UpdateWindows();
}

void App::Render()
{
    WindowManager::GetSingleton().RenderWindows();
}
