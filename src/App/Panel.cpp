#include "Panel.hpp"

#include "Display/WindowManager.hpp"
#include "Globals.hpp"
#include "System/Input/InputHandler.hpp"

Panel::Panel(const WindowInfo &info)
    : Window(info), pSceneWindow(nullptr), sceneWidthHeightRatio(4, 3),
      layout(Point2D(info.GetWidth(), info.GetHeight()))
{
}

bool Panel::Initialize(Window *pParent)
{
    if (!Window::Initialize(pParent))
        return false;
    const auto sceneAreaHeight = GetWindowInfo().border.height - PanelLayout::TopBarHeight;
    const auto sceneWindowWidth = sceneAreaHeight * sceneWidthHeightRatio.x / sceneWidthHeightRatio.y;
    WindowInfo childWndInfo(String(), false, sceneWindowWidth, sceneAreaHeight, WINDOW_HEIGHT - sceneAreaHeight,
                            sceneWindowWidth / 2);
    pSceneWindow = dynamic_cast<SceneWindow *>(
        WindowManager::GetSingleton().ConstructChildWindow<SceneWindow>(this, childWndInfo));
    return pSceneWindow && this->renderer.LoadGUILayout(layout);
}

void Panel::Render()
{
    Window::Render();
    this->renderer.Render(layout);
}

void Panel::OnWindowResized(long newWidth, long newHeight)
{
    Window::OnWindowResized(newWidth, newHeight);
    layout.SetWindowSize(Point2D(newWidth, newHeight));
    const auto sceneWindowHeight = newHeight - PanelLayout::TopBarHeight;
    const auto sceneWindowWidth = sceneWindowHeight * sceneWidthHeightRatio.x / sceneWidthHeightRatio.y;
    pSceneWindow->SetPosition((newWidth - sceneWindowWidth) / 2, PanelLayout::TopBarHeight);
    pSceneWindow->SetSize(sceneWindowWidth, sceneWindowHeight);
}

bool Panel::OnKeyboardInputReceived(const KeyCombination &keys)
{
    return Window::OnKeyboardInputReceived(keys);
}

bool Panel::OnMouseInputReceived(const MouseInfo &mouseInfo)
{
    return false;
}
