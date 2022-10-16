#include "Panel.hpp"

#include "Display/WindowManager.hpp"
#include "Globals.hpp"
#include "System/Input/InputHandler.hpp"

const unsigned long Panel::TopBarHeight = 50;

Panel::Panel(const WindowInfo &info) : Window(info), GUIScene(Scene::SceneType::GUI), pSceneWindow(nullptr)
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
    const auto sceneWindowWidth = WINDOW_WIDTH / 2;
    const auto sceneAreaHeight = WINDOW_HEIGHT - TopBarHeight;
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
    auto &info = GetWindowInfo();
    info.width = newWidth;
    info.height = newHeight;
    topBar->SetWindowSize(Point2D(newWidth, newHeight));
    pSceneWindow->SetPosition((newWidth - newHeight * 2 / 3) / 2, TopBarHeight);
    pSceneWindow->SetSize(newHeight * 2 / 3, newHeight * 2 / 3);
    Window::OnWindowResized(newWidth, newHeight);
}

bool Panel::OnKeyboardInputReceived(const KeyCombination &keys)
{
    return Window::OnKeyboardInputReceived(keys);
}

bool Panel::OnMouseInputReceived(const MouseInfo &mouseInfo)
{
    return false;
}
