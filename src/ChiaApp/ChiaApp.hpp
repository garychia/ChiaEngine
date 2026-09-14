#ifndef CHIA_APP_HPP
#define CHIA_APP_HPP

#include "App/App.hpp"
#include "SceneWindow.hpp"
#include "System/Module/CameraController.hpp"
#include "System/Module/Engine.hpp"
#include "System/Module/SimRecorder.hpp"
#include "System/Pong/PongHud.hpp"
#include "System/Pong/PongSystem.hpp"

// 組合根 — Pong demo 的 Sim/View split dogfood:
// Engine(固定步進) + SimRecorder(錄製/重播輸入) + PongSystem(確定性 Pong 世界)
// + PongHud(確定性 HUD 內容) + CameraController(固定相機 —— 不附著,避免與
// PongSystem 共用 actionBits 位元時移動衝突)。
// 附著順序 = 執行順序:recorder 先寫輸入,pong 後讀,hud 最後讀 pong 最新狀態。
class ChiaApp : public App
{
  private:
    Engine engine;

    SimRecorder simRecorder;

    pong::PongSystem pongSystem;

    pong::PongHud pongHud;

    CameraController cameraController; // 固定視角(0,0,16)→(0,180,0)正視球場

  public:
    ChiaApp(const AppInfo &info);

    virtual int Execute() override;

    virtual void Update() override;
};

#endif // CHIA_APP_HPP