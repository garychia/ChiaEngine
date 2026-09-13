#ifndef PANEL_HPP
#define PANEL_HPP

#include "Data/DynamicArray.hpp"
#include "Data/Pointers.hpp"
#include "SceneWindow.hpp"
#include "Components/PanelLayout.hpp"
#include "Display/GUI/EditorSession.hpp"
#include "Display/GUI/InspectorLayer.hpp"
#include "Display/GUI/Selection.hpp"
#include "System/Module/CameraController.hpp"
#include "System/Module/SimRecorder.hpp"
#include "System/World/Entity.hpp"

// ADR-0001 D4:Panel 是 View — 組合根是 EditorSession。
// ctor 把 SceneSystem*/CameraController*/SimRecorder* 注入 session,
// session 擁有 pane registry + selection + undo stack;這裡只做
// 視窗/GLFW 相關接線(子視窗定位、輸入轉發、inspector 刷新)。
class Panel : public Window
{
  private:
    SceneWindow *pSceneWindow;

    Point2D sceneWidthHeightRatio;

    // ADR-0001 D4:window-agnostic editor model(pointer set + pane registry
    // + selection + undo)。宣告在 layout 之前:layout ctor 需要它。
    EditorSession editorSession;

    PanelLayout layout;

    // ADR-0001 D2:單一真相來源,由 editorSession 擁有(InspectorLayer 只讀)。
    Selection &selection;

    Panel(const WindowInfo &info, SimRecorder *pSimRecorder, CameraController *pCameraController,
          SceneSystem *pSceneSystem);

    void OnHierarchyRowClicked(Entity entity);

    void RefreshHierarchyHighlight();

    // ADR-0001 D5-resize:從 window 尺寸 + scene ratio 計算全部 editor regions。
    PanelRegions ComputeRegions(long windowWidth, long windowHeight) const;

  public:
    virtual bool Initialize(Window *pParent = nullptr) override;

    virtual void Render() override;

    virtual void OnWindowResized(long newWidth, long newHeight) override;

    virtual bool OnKeyboardInputReceived(const KeyCombination &keys) override;

    virtual bool OnMouseInputReceived(const MouseInfo &mouseInfo) override;

    friend class WindowManager;
};

#endif // PANEL_HPP