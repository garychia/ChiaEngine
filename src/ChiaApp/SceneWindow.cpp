#include "SceneWindow.hpp"
#include "Display/Frame.hpp"
#include "Display/IRendererAssetRegistrar.hpp" // #83:資產註冊 seam(不再依賴 concrete VulkanRenderer)
#include "Display/Text/GlyphAtlas.hpp"
#include "Geometry/Primitives.hpp"
#include "System/Input/KeyCodes.hpp"

// #83 content-hash id:4-char ASCII 定址(拍/球的矩形幾何與固體色材質)。
static const uint64_t kQuadMeshId = 0x504F4E51ull;       // "PONQ"
static const uint64_t kPaddleMaterialId = 0x5050444Cull; // "PPDL"
static const uint64_t kBallMaterialId = 0x5042414Cull;   // "PBAL"

SceneWindow::SceneWindow(const WindowInfo &info, SimRecorder *pRecorder, CameraController *pController,
                         pong::PongSystem *pPong, pong::PongHud *pHud)
    : Window(info), pPong(pPong), pHud(pHud), pRecorder(pRecorder), pController(pController), pMainScene(),
      leftPaddleEntity(), rightPaddleEntity(), ballEntity(), replayKeyDown(false), restartKeyDown(false)
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
    if (pController)
        pMainScene->ApplyCamera(pController->GetCamera());
    // Sim 接縫:PongSystem 建立場上 entity(之後每 tick 由它推進;Reset 不銷毀 entity)。
    if (pPong)
    {
        leftPaddleEntity = pPong->CreateLeftPaddle();
        rightPaddleEntity = pPong->CreateRightPaddle();
        ballEntity = pPong->CreateBall();
    }
    return LoadScene(*pMainScene);
}

void SceneWindow::EnsureQuadAssetsRegistered()
{
    if (quadAssetsRegistered)
        return; // 只註冊一次(renderer 資源全域共用)
    quadAssetsRegistered = true;

    // #83:經由 IRendererAssetRegistrar seam 註冊資產,不再 dynamic_cast 到 concrete
    // VulkanRenderer(renderer 是 facade;facade 實作 IRendererAssetRegistrar 並 forward
    // 到底下 Vulkan executor,故 cast 成立)。
    IRendererAssetRegistrar *pRegistrar = dynamic_cast<IRendererAssetRegistrar *>(&renderer);
    if (!pRegistrar)
        return;

    // 幾何:單位 quad(XY 平面、中心原點、半寬/半高各 0.5)→ PushTransform scale 成實際尺寸。
    if (quadMeshId == 0)
    {
        auto quad = SharedPtr<IRenderable>::Construct<Rectangle>();
        if (pRegistrar->RegisterMeshGeometry(kQuadMeshId, quad->GetRenderInfo()))
            quadMeshId = kQuadMeshId;
    }

    // 材質:inline 1x1 RGBA(inline raw RGBA — 不需磁碟資產)。
    static const unsigned char kPaddleRgba[4] = {80, 200, 255, 255}; // 拍:固體淺藍
    MaterialSource paddleMat;
    paddleMat.pRawRGBA = kPaddleRgba;
    paddleMat.width = 1;
    paddleMat.height = 1;
    pRegistrar->RegisterMaterial(kPaddleMaterialId, paddleMat);

    static const unsigned char kBallRgba[4] = {255, 255, 255, 255}; // 球:白
    MaterialSource ballMat;
    ballMat.pRawRGBA = kBallRgba;
    ballMat.width = 1;
    ballMat.height = 1;
    pRegistrar->RegisterMaterial(kBallMaterialId, ballMat);
}

void SceneWindow::Render()
{
    // children(此視窗無子視窗)→ 略過
    // 首次執行時註冊 pong 資產(renderer 已初始化)。
    EnsureQuadAssetsRegistered();

    Frame frame;
    frame.BeginFrame();
    if (pController)
        frame.SetCamera(pController->GetCamera());

    // 場上幾何:拍/球位置讀自 Sim 世界(Sim 是狀態源,View 只投影)。
    if (pPong && quadMeshId != 0)
    {
        World &world = pPong->GetWorld();
        const pong::PaddleComponent *pLeft = world.GetComponent<pong::PaddleComponent>(leftPaddleEntity);
        const pong::PaddleComponent *pRight = world.GetComponent<pong::PaddleComponent>(rightPaddleEntity);
        const pong::BallComponent *pBall = world.GetComponent<pong::BallComponent>(ballEntity);

        // 兩根拍:quad 以 PaddleComponent.position 為中心,scale = (寬, 2×半高)。
        if (pLeft)
        {
            frame.BindMaterial(kPaddleMaterialId);
            frame.PushTransform(pLeft->position, Point3D(),
                                Point3D(2.0f * pong::PongSystem::PaddleHalfWidth, 2.0f * pLeft->halfHeight, 1.0f));
            frame.DrawMesh(quadMeshId);
            frame.PopTransform();
        }
        if (pRight)
        {
            frame.BindMaterial(kPaddleMaterialId);
            frame.PushTransform(pRight->position, Point3D(),
                                Point3D(2.0f * pong::PongSystem::PaddleHalfWidth, 2.0f * pRight->halfHeight, 1.0f));
            frame.DrawMesh(quadMeshId);
            frame.PopTransform();
        }

        // 球:小 quad,scale = 直徑(2×半徑)。
        if (pBall)
        {
            frame.BindMaterial(kBallMaterialId);
            frame.PushTransform(pBall->position, Point3D(), Point3D(2.0f * pong::PongSystem::BallRadius,
                                                                    2.0f * pong::PongSystem::BallRadius, 1.0f));
            frame.DrawMesh(quadMeshId);
            frame.PopTransform();
        }
    }

    // HUD:line.x/y 已是 NDC 左上錨點 → PushTransform 平移,再以 px→NDC scale
    // 縮放字形(與 GUIFrameProjector 同一慣例)。
    if (pHud)
    {
        const WindowInfo &win = GetWindowInfo();
        const float ndcScaleX = 2.0f / win.GetWidth();
        const float ndcScaleY = -2.0f / win.GetHeight();
        const DynamicArray<pong::PongHudLine> &lines = pHud->GetLines();
        for (size_t i = 0; i < lines.GetNElements(); i++)
        {
            const pong::PongHudLine &line = lines[i];
            frame.PushTransform(Point3D(line.x, line.y, 0.0f), Point3D(), Point3D(ndcScaleX, ndcScaleY, 1.0f));
            frame.DrawText(GlyphAtlas::DefaultFontId(), line.text, line.size, Color(line.r, line.g, line.b, line.a));
            frame.PopTransform();
        }
    }

    frame.EndFrame();
    renderer.Execute(frame);
}

bool SceneWindow::OnKeyboardInputReceived(const KeyCombination &combination)
{
    if (!pRecorder)
        return false;
    SimInput &input = pRecorder->GetLiveInput();

    // 從「目前按住的按鍵集合」直接推導 actionBits(stateless — 不需追蹤 press/release)。
    // W/S 或 ↑/↓ = 玩家拍;Space = 發球(PongSystem 以 BitLeft 觸發 launch);
    // R = 整局 Reset;F5/F6 = replay/live。
    input.actionBits = 0;
    bool hasF5 = false, hasF6 = false, hasR = false;
    for (size_t i = 0; i < combination.keys.Length(); i++)
    {
        switch (combination.keys[i])
        {
        case KeyCodeW:
        case KeyCodeUpArrow:
            input.actionBits |= pong::PongSystem::BitLeft;
            break;
        case KeyCodeS:
        case KeyCodeDownArrow:
            input.actionBits |= pong::PongSystem::BitRight;
            break;
        case KeyCodeSpace:
            input.actionBits |= pong::PongSystem::BitLeft; // 球未發射時觸發 launch
            break;
        case KeyCodeR:
            hasR = true;
            break;
        case KeyCodeF5:
            hasF5 = true;
            break;
        case KeyCodeF6:
            hasF6 = true;
            break;
        default:
            break;
        }
    }

    // R 邊緣:combination 含 R = 按下(ProcessKeyUp 已先移除 → 放開時不含)
    if (hasR && !restartKeyDown)
    {
        restartKeyDown = true;
        if (pPong)
            pPong->Reset();
    }
    else if (!hasR && restartKeyDown)
    {
        restartKeyDown = false;
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

    return input.actionBits != 0 || hasF5 || hasF6 || hasR;
}

bool SceneWindow::OnMouseInputReceived(const MouseInfo &mouseInfo)
{
    if (!pRecorder)
        return false;
    if (mouseInfo.leftButtonDown)
    {
        // look delta 累進 SimInput(本 demo 相機固定;保留軸向給 recorder/log)
        SimInput &input = pRecorder->GetLiveInput();
        input.axisX += mouseInfo.currentPosition.x - mouseInfo.lastMousePosition.x;
        input.axisY += mouseInfo.currentPosition.y - mouseInfo.lastMousePosition.y;
        return true;
    }
    return false;
}