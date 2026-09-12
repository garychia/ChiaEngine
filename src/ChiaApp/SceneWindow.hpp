#ifndef SCENE_WINDOW_HPP
#define SCENE_WINDOW_HPP

#include "Display/Window.hpp"
#include "Scene/SceneSystem.hpp"
#include "System/Module/CameraController.hpp"
#include "System/Module/SimRecorder.hpp"

// 3D viewport 視窗 — View 層:
// 真實鍵盤/滑鼠事件 → SimInput(錄進 SimRecorder),相機是 Sim 擁有的狀態
// (CameraController),這裡不再直接改相機。F5 = 從頭重播錄音,F6 = 回到 live。
class SceneWindow : public Window
{
  private:
    DynamicArray<SharedPtr<Texture>> pTextures;

    SharedPtr<Scene> pMainScene;

    SimRecorder *pRecorder;

    CameraController *pController;

    // #60 step 1:demo 節點階層建在這裡(Sim 側),hierarchy 側欄顯示
    SceneSystem *pSceneSystem;

    bool replayKeyDown; // F5 邊緣偵測(按下觸發一次)

    // #55:multi-material 示範 — 註冊延到 Render 首次執行(renderer 已 init)。
    bool materialsRegistered = false;
    uint64_t meshId_ = 0; // 已註冊的 cube meshId(0 = 未註冊)
    void EnsureMaterialDemoRegistered();

    SceneWindow(const WindowInfo &info, SimRecorder *pRecorder, CameraController *pController,
                SceneSystem *pSceneSystem);

  public:
    ~SceneWindow();

    virtual bool Initialize(Window *pParent = nullptr) override;

    virtual void Render() override;

    virtual bool OnKeyboardInputReceived(const KeyCombination &combination) override;

    virtual bool OnMouseInputReceived(const MouseInfo &mouseInfo) override;

    friend class WindowManager;
};

#endif // SCENE_WINDOW_HPP
