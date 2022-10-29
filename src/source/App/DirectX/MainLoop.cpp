#include "App/MainLoop.hpp"

MainLoop::MainLoop() : shouldContinue(true)
{
}

MainLoop::~MainLoop()
{
}

bool MainLoop::ShouldContinue() const
{
    return shouldContinue;
}

void MainLoop::Execute(WindowHandle mainWindowHandle)
{
    static MSG msg;
    msg = {};
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    shouldContinue = msg.message != WM_QUIT;
}
