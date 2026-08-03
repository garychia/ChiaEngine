#ifndef CHIA_APP_HPP
#define CHIA_APP_HPP

#include "App/App.hpp"
#include "Panel.hpp"
#include "System/Module/CameraController.hpp"
#include "System/Module/Engine.hpp"
#include "System/Module/SimRecorder.hpp"

// 組合根 — P4 Sim/View split 的 dogfood:
// Engine(固定步進) + SimRecorder(錄製/重播輸入) + CameraController(Sim 相機)。
// 附著順序 = 執行順序:recorder 先寫輸入,sim 後讀。
class ChiaApp : public App
{
  private:
    Engine engine;

    SimRecorder simRecorder;

    CameraController cameraController;

  public:
    ChiaApp(const AppInfo &info);

    virtual int Execute() override;

    virtual void Update() override;
};

#endif // CHIA_APP_HPP
