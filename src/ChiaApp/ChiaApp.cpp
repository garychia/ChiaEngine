#include "ChiaApp.hpp"

#include "Display/WindowManager.hpp"

#define DEFAULT_MAIN_WINDOW_WIDTH 1000
#define DEFAULT_MAIN_WINDOW_HEIGHT 800

ChiaApp::ChiaApp(const AppInfo &info)
    : App(info), engine(1), simRecorder(), pongSystem(), pongHud(),
      cameraController(Point3D(0.0f, 0.0f, 16.0f), Point3D(0.0f, 180.0f, 0.0f)) // 正視球場
{
    engine.Attach(&simRecorder); // 先附著:輸入先寫,sim 後讀
    engine.Attach(&pongSystem);
    engine.Attach(&pongHud);
    // cameraController 不附著 → FixedUpdate 不跑 → 相機固定、不吃 WASD 位元。
}

int ChiaApp::Execute()
{
    WindowInfo winInfo(&info, String("Chia Engine"), false, DEFAULT_MAIN_WINDOW_WIDTH,
                       DEFAULT_MAIN_WINDOW_HEIGHT);
    pMainWindow = WindowManager::GetSingleton().ConstructWindow<SceneWindow>(winInfo, &simRecorder,
                                                                             &cameraController, &pongSystem,
                                                                             &pongHud);
    if (!pMainWindow)
        return EXIT_FAILURE;
    if (!pMainWindow->Show())
        return EXIT_FAILURE;
    return App::Execute();
}

void ChiaApp::Update()
{
    // 輸入事件已在 mainLoop.Execute(glfwPollEvents) 寫進 SimInput;
    // 這裡先 tick sim(確定性),再更新視窗。Render 在 App::Render。
    engine.Tick(1.0 / 60.0);
    App::Update();
}