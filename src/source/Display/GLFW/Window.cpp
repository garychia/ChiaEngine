#include "Display/Window.hpp"
#include "Display/WindowManager.hpp"
#include "Display/GUI/GUILayout.hpp"

#include "System/Debug/Debug.hpp"
#include "System/Input/KeyCodes.hpp"
#include "System/Input/KeyboardHandler.hpp"
#include "System/Input/MouseInput.hpp"
#include "pch.hpp"

namespace
{
// GLFW key → 引擎 KeyCode(對應 Windows 版 WinKeyCode.hpp 的 ConvertToKeyCode)
KeyCode ConvertGlfwKeyToKeyCode(int key)
{
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
        return static_cast<KeyCode>(KeyCodeA + (key - GLFW_KEY_A));
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
        return static_cast<KeyCode>(KeyCodeZero + (key - GLFW_KEY_0));
    if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12)
        return static_cast<KeyCode>(KeyCodeF1 + (key - GLFW_KEY_F1));
    switch (key)
    {
    case GLFW_KEY_ESCAPE:
        return KeyCodeEscape;
    case GLFW_KEY_SPACE:
        return KeyCodeSpace;
    case GLFW_KEY_ENTER:
        return KeyCodeReturn;
    case GLFW_KEY_TAB:
        return KeyCodeTab;
    case GLFW_KEY_BACKSPACE:
        return KeyCodeBackspace;
    case GLFW_KEY_LEFT:
        return KeyCodeLeftArrow;
    case GLFW_KEY_RIGHT:
        return KeyCodeRightArrow;
    case GLFW_KEY_UP:
        return KeyCodeUpArrow;
    case GLFW_KEY_DOWN:
        return KeyCodeDownArrow;
    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_RIGHT_SHIFT:
        return KeyCodeShift;
    case GLFW_KEY_LEFT_CONTROL:
    case GLFW_KEY_RIGHT_CONTROL:
        return KeyCodeControl;
    case GLFW_KEY_LEFT_ALT:
    case GLFW_KEY_RIGHT_ALT:
        return KeyCodeALT;
    default:
        return KeyCodeUndefined;
    }
}

// GLFW 輸入事件 → 既有輸入管線(KeyboardHandler / MouseInput → WindowManager::Handle*Input)
void ChiaKeyCallback(GLFWwindow *pWindow, int key, int /*scancode*/, int action, int /*mods*/)
{
    const KeyCode code = ConvertGlfwKeyToKeyCode(key);
    KeyboardHandler &handler = KeyboardHandler::GetSingleton();
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
        handler.ProcessKeyDown(code);
    else if (action == GLFW_RELEASE)
        handler.ProcessKeyUp(code);
    if (code != KeyCodeUndefined)
        WindowManager::GetSingleton().HandleKeyInput(pWindow, handler.GetKeyCombination());
}

void ChiaCursorPosCallback(GLFWwindow *pWindow, double x, double y)
{
    MouseInput &mouse = MouseInput::GetSingleton();
    mouse.OnMouseMove(static_cast<float>(x), static_cast<float>(y));
    WindowManager::GetSingleton().HandleMouseInput(pWindow, mouse.GetMouseInfo());
}

void ChiaMouseButtonCallback(GLFWwindow *pWindow, int button, int action, int /*mods*/)
{
    double x = 0.0, y = 0.0;
    glfwGetCursorPos(pWindow, &x, &y);
    MouseInput &mouse = MouseInput::GetSingleton();
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
            mouse.OnMouseLeftButtonDown(static_cast<float>(x), static_cast<float>(y));
        else if (action == GLFW_RELEASE)
            mouse.OnMouseLeftButtonUp(static_cast<float>(x), static_cast<float>(y));
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
            mouse.OnMouseRightButtonDown(static_cast<float>(x), static_cast<float>(y));
        else if (action == GLFW_RELEASE)
            mouse.OnMouseRightButtonUp(static_cast<float>(x), static_cast<float>(y));
    }
    WindowManager::GetSingleton().HandleMouseInput(pWindow, mouse.GetMouseInfo());
}

void ChiaScrollCallback(GLFWwindow *pWindow, double /*xOffset*/, double yOffset)
{
    MouseInput &mouse = MouseInput::GetSingleton();
    mouse.OnWheelRotated(static_cast<int>(yOffset * 120)); // 對齊 Windows WHEEL_DELTA
    WindowManager::GetSingleton().HandleMouseInput(pWindow, mouse.GetMouseInfo());
}
} // namespace

Window::Window(const WindowInfo &info)
    : handle(NULL), pParent(nullptr), info(info), pScene(nullptr), pGUILayout(nullptr),
      renderer(), pChildren()
{
}

DynamicArray<Window *> &Window::GetChildren()
{
    return pChildren;
}

const DynamicArray<Window *> &Window::GetChildren() const
{
    return pChildren;
}

bool Window::AddChild(Window *pChild)
{
    pChildren.Append(pChild);
    return true;
}

Window::~Window()
{
    Destroy();
}

bool Window::Initialize(Window *pParent)
{
    this->pParent = pParent;
    info.fullScreen = info.fullScreen && pParent;
    return true;
}

bool Window::LoadScene(Scene &scene)
{
    this->pScene = &scene;
    scene.onCameraChanged.Subscribe(this, &Window::OnCameraChanged);
    return renderer.LoadScene(scene);
}

bool Window::Show()
{
    char title[1024];
    info.title.ToUTF8(title, sizeof(title));
    handle = glfwCreateWindow(info.GetWidth(), info.GetHeight(), title,
                              info.fullScreen ? glfwGetPrimaryMonitor() : NULL, NULL);
    if (!handle)
        return false;
    glfwSetWindowPos(handle, static_cast<int>(info.border.xPos), static_cast<int>(info.border.yPos));
    // 輸入事件 → 既有輸入管線(GLFW 版原本缺這條,鍵盤/滑鼠都是死的)
    glfwSetKeyCallback(handle, ChiaKeyCallback);
    glfwSetCursorPosCallback(handle, ChiaCursorPosCallback);
    glfwSetMouseButtonCallback(handle, ChiaMouseButtonCallback);
    glfwSetScrollCallback(handle, ChiaScrollCallback);
    // 以真實 handle 註冊進 WindowManager:Construct 階段 GetHandle() 還是 NULL,
    // 缺這行任何輸入 callback 查 windowMap 都會缺 key 插入 null → 解引用崩潰
    // (鼠標一進視窗就閃退,issue #38)
    WindowManager::GetSingleton().RegisterWindow(this);
    // renderer 需要 GLFW window handle 才能建立 surface / swapchain
    if (!renderer.Initialize(this))
        return false;
    // 子視窗(如 SceneWindow)需要自己的 GLFW window + renderer,否則 Execute 會
    // 因 device 未初始化而跳過(畫面只剩父視窗的清色,場景永遠畫不出來)
    for (size_t i = 0; i < pChildren.Length(); i++)
    {
        if (!pChildren[i]->Show())
            return false;
    }
    return true;
}

void Window::Update()
{
    for (size_t i = 0; i < pChildren.Length(); i++)
        pChildren[i]->Update();
    renderer.Update();
}

void Window::Render()
{
    for (size_t i = 0; i < pChildren.Length(); i++)
        pChildren[i]->Render();

    // Frame 驅動的渲染:錄製命令 → executor 執行
    // P6:場景與 GUI 都走 Frame(唯一貨幣)— GUI 由 SetGUILayout 掛上的佈局錄進命令流。
    Frame frame;
    frame.BeginFrame();
    if (pScene)
    {
        frame.SetCamera(pScene->GetCamera());
        const DynamicArray<SharedPtr<IRenderable>> &renderables = pScene->GetRenderables();
        for (size_t i = 0; i < renderables.GetNElements(); i++)
            frame.DrawRenderable(*renderables[i]);
    }
    if (pGUILayout)
        frame.DrawGUILayout(*pGUILayout);
    frame.EndFrame();
    renderer.Execute(frame);
}

void Window::SetGUILayout(GUILayout *pLayout)
{
    this->pGUILayout = pLayout;
}

WindowHandle Window::GetHandle() const
{
    return handle;
}

Window *Window::GetParent() const
{
    return pParent;
}

WindowInfo &Window::GetWindowInfo()
{
    return info;
}

const WindowInfo &Window::GetWindowInfo() const
{
    return info;
}

void Window::SetPosition(unsigned long newX, unsigned long newY)
{
    info.border.xPos = newX;
    info.border.yPos = newY;
    glfwSetWindowPos(GetHandle(), newX, newY);
}

void Window::SetSize(unsigned long newWidth, unsigned long newHeight)
{
    info.border.width = newWidth;
    info.border.height = newHeight;
    glfwSetWindowSize(GetHandle(), newWidth, newHeight);
}

void Window::Destroy()
{
    for (size_t i = 0; i < pChildren.Length(); i++)
        pChildren[i]->Destroy();
    if (handle)
        glfwDestroyWindow(handle);
    handle = nullptr;
}

void Window::OnCameraChanged(WeakPtr<Camera> &pCamera)
{
    renderer.ApplyCamera(pCamera);
}

void Window::OnWindowResized(long newWidth, long newHeight)
{
    info.border.width = newWidth;
    info.border.height = newHeight;
    renderer.OnWindowResized(newWidth, newHeight);
}

bool Window::OnKeyboardInputReceived(const KeyCombination &keys)
{
    for (size_t i = 0; i < pChildren.Length(); i++)
    {
        if (pChildren[i]->OnKeyboardInputReceived(keys))
            return true;
    }
    return false;
}

bool Window::OnMouseInputReceived(const MouseInfo &mouseInfo)
{
    for (size_t i = 0; i < pChildren.Length(); i++)
    {
        if (pChildren[i]->OnMouseInputReceived(mouseInfo))
            return true;
    }
    return false;
}
