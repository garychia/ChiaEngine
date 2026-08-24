#include "Panel.hpp"

#include "Display/WindowManager.hpp"
#include "System/Input/InputHandler.hpp"

Panel::Panel(const WindowInfo &info, SimRecorder *pSimRecorder, CameraController *pCameraController,
             SceneSystem *pSceneSystem)
    : Window(info), pSceneWindow(nullptr), sceneWidthHeightRatio(4, 3),
      layout(Point2D(info.GetWidth(), info.GetHeight())), pSimRecorder(pSimRecorder),
      pCameraController(pCameraController), pSceneSystem(pSceneSystem), selectedEntity()
{
}

bool Panel::Initialize(Window *pParent)
{
    if (!Window::Initialize(pParent))
        return false;
    const auto sceneAreaHeight = GetWindowInfo().GetHeight() - PanelLayout::TopBarHeight;
    const auto sceneWindowWidth = sceneAreaHeight * sceneWidthHeightRatio.x / sceneWidthHeightRatio.y;
    WindowInfo childWndInfo(info.pAppInfo, String(), false, sceneWindowWidth, sceneAreaHeight,
                            GetWindowInfo().GetHeight() - sceneAreaHeight, sceneWindowWidth / 2);
    pSceneWindow = dynamic_cast<SceneWindow *>(
        WindowManager::GetSingleton().ConstructChildWindow<SceneWindow>(this, childWndInfo,
                                                                        pSimRecorder, pCameraController,
                                                                        pSceneSystem));
    // P6:GUI 走 Frame — 佈局掛上視窗,由 Window::Render 錄成 DrawGUILayout 命令。
    // (取代 legacy renderer.LoadGUILayout/Render(layout),該路徑在 Vulkan 下是空實作,
    //  top bar 從未真正畫出來。Windows DX 仍走 legacy,不受影響。)
    if (!pSceneWindow)
        return false;
    // #60 step 1:SceneWindow::Initialize 已建立 demo 節點,這裡重建側欄並接選取事件。
    layout.BuildHierarchy(*pSceneSystem);
    auto &rows = layout.GetHierarchyRows();
    for (size_t i = 0; i < rows.GetNElements(); i++)
        rows[i]->rowClicked.Subscribe(this, &Panel::OnHierarchyRowClicked);
    // #60 step 2:建立右側 Inspector(消費選取的 entity)。
    layout.CreateInspector(*pSceneSystem);
    SetGUILayout(&layout);
    return true;
}

void Panel::Render()
{
    // 佈局經 Window::Render 的 Frame 命令流繪製(DrawGUILayout),不再走 legacy 死路。
    // Inspector 每幀重畫選取 entity 的目前值(反映外部 transform 改動)。
    if (InspectorLayer *pInspector = layout.GetInspector())
        pInspector->Update();
    Window::Render();
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
    // #64:轉發給 base — children(場景)先處理,GUI hit-test 最後;
    // 點到 toolbar 元件即消費,點到場景空白處維持原本 return false。
    return Window::OnMouseInputReceived(mouseInfo);
}

void Panel::OnHierarchyRowClicked(Entity entity)
{
    selectedEntity = entity;
    if (InspectorLayer *pInspector = layout.GetInspector())
        pInspector->SelectEntity(entity.GetIndex());
    RefreshHierarchyHighlight();
}

void Panel::RefreshHierarchyHighlight()
{
    auto &rows = layout.GetHierarchyRows();
    for (size_t i = 0; i < rows.GetNElements(); i++)
    {
        if (rows[i]->GetEntity() == selectedEntity)
            rows[i]->SetColor(Color(0.35f, 0.45f, 0.85f)); // 選取高亮
        else
            rows[i]->SetColor(Color(0.22f, 0.22f, 0.25f));
    }
}
