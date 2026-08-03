#include "SceneWindow.hpp"
#include "Paths.hpp"
#include "Geometry/Primitives.hpp"

SceneWindow::SceneWindow(const WindowInfo &info, SimRecorder *pRecorder, CameraController *pController)
    : Window(info), pTextures(), pMainScene(), pRecorder(pRecorder), pController(pController),
      replayKeyDown(false)
{
    pMainScene = SharedPtr<Scene>::Construct();
}

SceneWindow::~SceneWindow()
{
}

bool SceneWindow::Initialize(Window *pParent)
{
    if (!Window::Initialize(pParent))
        return false;
    auto cube = SharedPtr<IRenderable>::Construct<Cube>();
    auto pTexture = SharedPtr<Texture>::Construct(String(IMAGE_FILE_PATH) + "michael-sum-unsplash.jpg");
    pTextures.Append(pTexture);
    cube->SetTexture(pTexture.GetRaw());
    pMainScene->AddRenderable(cube);
    // 相機是 Sim 擁有的狀態(CameraController),View 只拿 WeakPtr 來渲染
    if (pController)
        pMainScene->ApplyCamera(pController->GetCamera());
    return LoadScene(*pMainScene);
}

bool SceneWindow::OnKeyboardInputReceived(const KeyCombination &combination)
{
    if (!pRecorder)
        return false;
    SimInput &input = pRecorder->GetLiveInput();

    // 從「目前按住的按鍵集合」直接推導 actionBits(stateless — 不需追蹤 press/release)
    input.actionBits = 0;
    bool hasF5 = false, hasF6 = false;
    for (size_t i = 0; i < combination.keys.Length(); i++)
    {
        switch (combination.keys[i])
        {
        case KeyCode::KeyCodeW:
            input.actionBits |= CameraController::BitMoveForward;
            break;
        case KeyCode::KeyCodeS:
            input.actionBits |= CameraController::BitMoveBack;
            break;
        case KeyCode::KeyCodeA:
            input.actionBits |= CameraController::BitMoveLeft;
            break;
        case KeyCode::KeyCodeD:
            input.actionBits |= CameraController::BitMoveRight;
            break;
        case KeyCode::KeyCodeF5:
            hasF5 = true;
            break;
        case KeyCode::KeyCodeF6:
            hasF6 = true;
            break;
        default:
            break;
        }
    }

    // F5 邊緣:combination 含 F5 = 按下(ProcessKeyUp 已先移除 → 放開時不含)
    if (hasF5 && !replayKeyDown)
    {
        replayKeyDown = true;
        if (pController)
            pController->Reset();
        pRecorder->BeginReplay();
    }
    else if (!hasF5 && replayKeyDown)
    {
        replayKeyDown = false;
    }
    if (hasF6)
        pRecorder->SetReplaying(false);

    return input.actionBits != 0 || hasF5 || hasF6;
}

bool SceneWindow::OnMouseInputReceived(const MouseInfo &mouseInfo)
{
    if (!pRecorder)
        return false;
    if (mouseInfo.leftButtonDown)
    {
        // look delta 累進 SimInput;CameraController 每 tick 讀完即消耗歸零
        SimInput &input = pRecorder->GetLiveInput();
        input.axisX += mouseInfo.currentPosition.x - mouseInfo.lastMousePosition.x;
        input.axisY += mouseInfo.currentPosition.y - mouseInfo.lastMousePosition.y;
        return true;
    }
    return false;
}
