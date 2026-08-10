#ifndef PANEL_HPP
#define PANEL_HPP

#include "Data/DynamicArray.hpp"
#include "Data/Pointers.hpp"
#include "SceneWindow.hpp"
#include "Components/PanelLayout.hpp"
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

    // editor 選取狀態 — step 2 的 Inspector 消費
    Entity selectedEntity;

    Panel(const WindowInfo &info, SimRecorder *pSimRecorder, CameraController *pCameraController,
          SceneSystem *pSceneSystem);

    void OnHierarchyRowClicked(Entity entity);

    void RefreshHierarchyHighlight();

  public:
    virtual bool Initialize(Window *pParent = nullptr) override;

    virtual void Render() override;

    virtual void OnWindowResized(long newWidth, long newHeight) override;

    virtual bool OnKeyboardInputReceived(const KeyCombination &keys) override;

    virtual bool OnMouseInputReceived(const MouseInfo &mouseInfo) override;

    friend class WindowManager;
};

#endif // PANEL_HPP
