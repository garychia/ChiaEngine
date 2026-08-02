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
#ifdef DIRECTX_ENABLED
    // DirectX backend:Win32 訊息泵
    static MSG msg;
    msg = {};
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    shouldContinue = msg.message != WM_QUIT;
#else
    // GLFW backends (Vulkan / OpenGL):原生事件輪詢
    glfwPollEvents();
    shouldContinue = !glfwWindowShouldClose(mainWindowHandle);
#endif
}
