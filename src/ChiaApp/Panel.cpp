#include "Panel.hpp"

#include "Display/WindowManager.hpp"
#include "System/Input/InputHandler.hpp"

Panel::Panel(const WindowInfo &info, SimRecorder *pSimRecorder, CameraController *pCameraController,
             SceneSystem *pSceneSystem)
    : Window(info), pSceneWindow(nullptr), sceneWidthHeightRatio(4, 3),
      layout(Point2D(info.GetWidth(), info.GetHeight())), pSimRecorder(pSimRecorder),
      pCameraController(pCameraController), pSceneSystem(pSceneSystem),
      editorSession(), selection(editorSession.GetSelection())
{
}

bool Panel::Initialize(Window *pParent)
{
    if (!Window::Initialize(pParent))
        return false;
    const long w = GetWindowInfo().GetWidth();
    const long h = GetWindowInfo().GetHeight();
    const PanelRegions regions = ComputeRegions(w, h);
    WindowInfo childWndInfo(info.pAppInfo, String(), false, static_cast<unsigned long>(regions.sceneSize.x),
                            static_cast<unsigned long>(regions.sceneSize.y),
                            GetWindowInfo().GetHeight() - PanelLayout::TopBarHeight,
                            static_cast<unsigned long>(regions.centerViewport.xPos));
    pSceneWindow = dynamic_cast<SceneWindow *>(
        WindowManager::GetSingleton().ConstructChildWindow<SceneWindow>(this, childWndInfo,
                                                                        pSimRecorder, pCameraController,
                                                                        pSceneSystem));
    // P6:GUI 走 Frame — 佈局掛上視窗,由 Window::Render 錄成 DrawGUILayout 命令。
    // (取代 legacy renderer.LoadGUILayout/Render(layout),該路徑在 Vulkan 下是空實作,
    //  top bar 從未真正畫出來。Windows DX 仍走 legacy,不受影響。)
    if (!pSceneWindow)
        return false;
    // 初始位置/尺寸已由 childWndInfo(border)決定;不要在 Initialize 階段
    // SetPosition/SetSize — GLFW handle 還沒建立(Show() 才建),呼叫會 assert。
    // (OnWindowResized 才是 resize 後重新定位的時機。)
    // #60 step 1:SceneWindow::Initialize 已建立 demo 節點,這裡重建側欄並接選取事件。
    layout.BuildHierarchy(*pSceneSystem);
    auto &rows = layout.GetHierarchyRows();
    for (size_t i = 0; i < rows.GetNElements(); i++)
        rows[i]->rowClicked.Subscribe(this, &Panel::OnHierarchyRowClicked);
    // #60 step 2:建立右側 Inspector(消費選取的 entity),按鈕 push 到 session undo stack。
    layout.CreateInspector(*pSceneSystem, &editorSession.GetUndoStack());
    layout.SetRegions(Point2D(static_cast<float>(w), static_cast<float>(h)), regions);
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
    const PanelRegions regions = ComputeRegions(newWidth, newHeight);
    layout.SetRegions(Point2D(static_cast<float>(newWidth), static_cast<float>(newHeight)), regions);
    pSceneWindow->SetPosition(static_cast<long>(regions.centerViewport.xPos),
                              static_cast<long>(regions.centerViewport.yPos));
    pSceneWindow->SetSize(static_cast<long>(regions.sceneSize.x), static_cast<long>(regions.sceneSize.y));
}

PanelRegions Panel::ComputeRegions(long windowWidth, long windowHeight) const
{
    return ComputePanelRegions(windowWidth, windowHeight, sceneWidthHeightRatio);
}

bool Panel::OnKeyboardInputReceived(const KeyCombination &keys)
{
    // ADR-0001 D5:editor 快捷鍵 — Ctrl+Z undo、Ctrl+Y redo(與 SimRecorder 的
    // F5/F6 replay 完全分離:這是 editor-time edit,不是 gameplay replay)。
    // 消費掉,不再轉發給場景(避免 WASD 同時觸發)。
    bool hasCtrl = false;
    bool hasZ = false;
    bool hasY = false;
    for (size_t i = 0; i < keys.keys.Length(); i++)
    {
        switch (keys.keys[i])
        {
            case KeyCodeControl: hasCtrl = true; break;
            case KeyCodeZ: hasZ = true; break;
            case KeyCodeY: hasY = true; break;
            default: break;
        }
    }
    if (hasCtrl && hasZ)
    {
        editorSession.GetUndoStack().Undo();
        if (InspectorLayer *pInspector = layout.GetInspector())
            pInspector->Update();
        return true;
    }
    if (hasCtrl && hasY)
    {
        editorSession.GetUndoStack().Redo();
        if (InspectorLayer *pInspector = layout.GetInspector())
            pInspector->Update();
        return true;
    }
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
    editorSession.SetSelection(entity.GetIndex(), true);
    if (InspectorLayer *pInspector = layout.GetInspector())
        pInspector->SetSelection(&selection);
    RefreshHierarchyHighlight();
}

void Panel::RefreshHierarchyHighlight()
{
    auto &rows = layout.GetHierarchyRows();
    for (size_t i = 0; i < rows.GetNElements(); i++)
    {
        const bool isSelected = selection.hasSelection &&
                                rows[i]->GetEntity().GetIndex() == selection.entityIndex;
        if (isSelected)
            rows[i]->SetColor(Color(0.35f, 0.45f, 0.85f)); // 選取高亮
        else
            rows[i]->SetColor(Color(0.22f, 0.22f, 0.25f));
    }
}
