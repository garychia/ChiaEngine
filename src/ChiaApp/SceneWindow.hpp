#ifndef SCENE_WINDOW_HPP
#define SCENE_WINDOW_HPP

#include "Display/Window.hpp"
#include "System/Module/CameraController.hpp"
#include "System/Module/SimRecorder.hpp"
#include "System/Pong/PongHud.hpp"
#include "System/Pong/PongSystem.hpp"
#include "System/World/Entity.hpp"

// 3D viewport 視窗 — View 層(Pong demo):
// 真實鍵盤事件 → SimInput(錄進 SimRecorder),Sim 的 PongSystem 每 tick 推進世界,
// 這裡每幀把世界(PaddleComponent/BallComponent)投影成 Frame 的 DrawMesh,
// 並把 PongHud 的確定性 HUD 內容(PongHudLine)投影成 DrawText。相機是 Sim 擁有的
// 狀態(CameraController)— 本 demo 固定,不讀輸入。F5 = 從頭重播錄音,F6 = 回到
// live,R = PongSystem::Reset()(整局重開)。
class SceneWindow : public Window
{
  private:
    pong::PongSystem *pPong;

    pong::PongHud *pHud;

    SimRecorder *pRecorder;

    CameraController *pController;

    SharedPtr<Scene> pMainScene;

    // PongSystem 接縫建立的 entity(每幀讀世界取位置)
    Entity leftPaddleEntity;
    Entity rightPaddleEntity;
    Entity ballEntity;

    bool replayKeyDown;  // F5 邊緣偵測(按下觸發一次)
    bool restartKeyDown; // R 邊緣偵測(按下觸發一次 Reset)

    // 註冊延到 Render 首次執行(renderer 已 init)的 quad 資產。
    bool quadAssetsRegistered = false;
    uint64_t quadMeshId = 0; // 已註冊的 quad meshId(0 = 未註冊)
    void EnsureQuadAssetsRegistered();

    SceneWindow(const WindowInfo &info, SimRecorder *pRecorder, CameraController *pController,
                pong::PongSystem *pPong, pong::PongHud *pHud);

  public:
    ~SceneWindow();

    virtual bool Initialize(Window *pParent = nullptr) override;

    virtual void Render() override;

    virtual bool OnKeyboardInputReceived(const KeyCombination &combination) override;

    virtual bool OnMouseInputReceived(const MouseInfo &mouseInfo) override;

    friend class WindowManager;
};

#endif // SCENE_WINDOW_HPP