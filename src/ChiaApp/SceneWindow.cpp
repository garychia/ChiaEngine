#include "SceneWindow.hpp"
#include "Paths.hpp"
#include "Display/Frame.hpp"
#include "Display/IRendererAssetRegistrar.hpp" // #83:資產註冊 seam(不再依賴 concrete VulkanRenderer)
#include "Geometry/Primitives.hpp"

SceneWindow::SceneWindow(const WindowInfo &info, SimRecorder *pRecorder, CameraController *pController,
                         SceneSystem *pSceneSystem)
    : Window(info), pTextures(), pMainScene(), pRecorder(pRecorder), pController(pController),
      pSceneSystem(pSceneSystem), replayKeyDown(false)
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
    // #55:多材質示範 — 幾何/Material 註冊延到 Render() 首次執行時
    // (Window::Show → renderer.Initialize 之後才能建 GPU 資源);
    // 此處保留 legacy Scene 供既有 path 相容。
    auto cube = SharedPtr<IRenderable>::Construct<Cube>();
    auto pTexture = SharedPtr<Texture>::Construct(String(IMAGE_FILE_PATH) + "michael-sum-unsplash.jpg");
    pTextures.Append(pTexture);
    cube->SetTexture(pTexture.GetRaw());
    pMainScene->AddRenderable(cube);
    // 相機是 Sim 擁有的狀態(CameraController),View 只拿 WeakPtr 來渲染
    if (pController)
        pMainScene->ApplyCamera(pController->GetCamera());
    // #60 step 1 demo:建立節點階層(Sim 側)供 hierarchy 側欄顯示。
    // 純 editor 演示資料 — 之後的 step 2/3 才把節點綁到 renderable。
    if (pSceneSystem)
    {
        Entity root = pSceneSystem->CreateNode();
        Entity childA = pSceneSystem->CreateNode(root);
        pSceneSystem->CreateNode(root);
        pSceneSystem->CreateNode(childA);
    }
    return LoadScene(*pMainScene);
}

void SceneWindow::EnsureMaterialDemoRegistered()
{
    if (materialsRegistered)
        return; // 只註冊一次(renderer 資源全域共用)
    materialsRegistered = true;

    // #83:經由 IRendererAssetRegistrar seam 註冊資產,不再 dynamic_cast 到 concrete
    // VulkanRenderer。renderer 是 Renderer facade;facade 實作 IRendererAssetRegistrar
    // 並 forward 到底下 Vulkan executor,故 cast 成立、多材質示範真正註冊。
    // (修復 #55 的隱性 bug:舊代碼 dynamic_cast<VulkanRenderer*>(&renderer) 對
    // facade 永遠回 null,多材質示範因此從未真正註冊、悄悄退回 legacy path。)
    IRendererAssetRegistrar *pRegistrar = dynamic_cast<IRendererAssetRegistrar *>(&renderer);
    if (!pRegistrar)
        return;

    // 幾何:content-hash meshId(固定常數;真實系統由 AssetManager 內容定址給)。
    if (meshId_ == 0)
    {
        const uint64_t kCubeMeshId = 0x43554245ull; // "CUBE"
        auto cube = SharedPtr<IRenderable>::Construct<Cube>();
        if (pRegistrar->RegisterMeshGeometry(kCubeMeshId, cube->GetRenderInfo()))
            meshId_ = kCubeMeshId;
    }

    // 材質 1:原本的貓 JPG(磁碟資產,stbi 載入)。
    MaterialSource mat1;
    mat1.pTexture = pTextures.GetFirst().GetRaw();
    pRegistrar->RegisterMaterial(0x4D415431ull /* "MAT1" */, mat1);

    // 材質 2:inline RGBA 棋盤格(2x2,紅/暗紅)— 展示 per-material texture 不需磁碟資產。
    static const unsigned char kChecker[2 * 2 * 4] = {
        255, 60, 40, 255, 120, 20, 15, 255,
        120, 20, 15, 255, 255, 60, 40, 255,
    };
    MaterialSource mat2;
    mat2.pRawRGBA = kChecker;
    mat2.width = 2;
    mat2.height = 2;
    pRegistrar->RegisterMaterial(0x4D415432ull /* "MAT2" */, mat2);
}

void SceneWindow::Render()
{
    // children(此視窗無子視窗)→ 略過
    // 首次執行時註冊 multi-material 示範(renderer 已初始化)。
    EnsureMaterialDemoRegistered();

    Frame frame;
    frame.BeginFrame();
    if (pController)
        frame.SetCamera(pController->GetCamera());
    // #55:走 DrawMesh + BindMaterial 路徑 — 2 顆 cube、2 種材質。
    // #83:不再 dynamic_cast 到 concrete VulkanRenderer(對 facade 永遠 null)。
    // meshId_ != 0 即代表資產已透過 IRendererAssetRegistrar seam 註冊成功。
    if (meshId_ != 0)
    {
        // 材質 1 cube(左):貓 JPG。
        frame.BindMaterial(0x4D415431ull);
        frame.PushTransform(Point3D(-1.2f, 0.0f, 0.0f), Point3D(), Point3D(0.9f));
        frame.DrawMesh(meshId_);
        frame.PopTransform();
        // 材質 2 cube(右):紅色棋盤格。
        frame.BindMaterial(0x4D415432ull);
        frame.PushTransform(Point3D(1.2f, 0.0f, 0.0f), Point3D(), Point3D(0.9f));
        frame.DrawMesh(meshId_);
        frame.PopTransform();
    }
    else
    {
        // fallback:legacy renderable 路徑(renderer 非 Vulkan 或註冊失敗)。
        if (pMainScene)
        {
            const DynamicArray<SharedPtr<IRenderable>> &renderables = pMainScene->GetRenderables();
            for (size_t i = 0; i < renderables.GetNElements(); i++)
                frame.DrawRenderable(*renderables[i]);
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
