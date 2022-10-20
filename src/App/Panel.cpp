#include "Panel.hpp"

#include "Display/WindowManager.hpp"
#include "Globals.hpp"
#include "System/Input/InputHandler.hpp"

const unsigned long Panel::TopBarHeight = 30;

Panel::Panel(const WindowInfo &info)
    : Window(info), GUIScene(Scene::SceneType::GUI), pSceneWindow(nullptr), sceneWidthHeightRatio(4, 3)
{
    topBar = SharedPtr<HorizontalList>::Construct(Point2D(info.width, info.height),
                                                  Border(0.f, 0.f, {100.f, true}, TopBarHeight));
    GUIScene.AddRenderable(topBar->GetRenderable());
}

Panel::~Panel()
{
}

bool Panel::Initialize(Window *pParent)
{
    if (!Window::Initialize(pParent))
        return false;
    const auto sceneAreaHeight = GetWindowInfo().height - TopBarHeight;
    const auto sceneWindowWidth = sceneAreaHeight * sceneWidthHeightRatio.x / sceneWidthHeightRatio.y;
    WindowInfo childWndInfo(String(), false, sceneWindowWidth, sceneAreaHeight, WINDOW_HEIGHT - sceneAreaHeight,
                            sceneWindowWidth / 2);
    pSceneWindow = dynamic_cast<SceneWindow *>(
        WindowManager::GetSingleton().ConstructChildWindow<SceneWindow>(this, childWndInfo));
    return pSceneWindow && LoadScene(GUIScene);
}

void Panel::Render()
{
    this->renderer.Render(GUIScene);
    Window::Render();
}

void Panel::OnWindowResized(long newWidth, long newHeight)
{
    Window::OnWindowResized(newWidth, newHeight);
    topBar->SetWindowSize(Point2D(newWidth, newHeight));
    const auto sceneWindowHeight = newHeight - TopBarHeight;
    const auto sceneWindowWidth = sceneWindowHeight * sceneWidthHeightRatio.x / sceneWidthHeightRatio.y;
    pSceneWindow->SetPosition((newWidth - sceneWindowWidth) / 2, TopBarHeight);
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
