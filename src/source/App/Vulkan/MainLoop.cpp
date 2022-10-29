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
    glfwPollEvents();
    shouldContinue = !glfwWindowShouldClose(mainWindowHandle);
}
