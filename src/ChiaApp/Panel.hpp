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

class Panel : public Window
{
  private:
    SceneWindow *pSceneWindow;

    Point2D sceneWidthHeightRatio;

    PanelLayout layout;

    SimRecorder *pSimRecorder;

    CameraController *pCameraController;

    // #60 step 1:hierarchy 側欄的資料源(Sim 側場景圖)
    SceneSystem *pSceneSystem;

    // ADR-0001 D4:window-agnostic editor state (selection + undo/redo).
    EditorSession editorSession;

    // #60 step 1:選取狀態(ADR-0001 D2:單一真相來源,InspectorLayer 只讀),
    // 由 editorSession 擁有。
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
