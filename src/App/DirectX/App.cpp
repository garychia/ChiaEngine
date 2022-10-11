#include "App/App.hpp"
#include "Geometry/Primitives.hpp"
#include "Globals.hpp"
#include "Paths.hpp"
#include "System/Debug/Debug.hpp"
#include "System/Input/InputHandler.hpp"
#include "Display/WindowManager.hpp"

bool App::Initialize()
{
    MainWindow = nullptr;
    FullScreen = false;
    AppName = String(APPLICATION_NAME);
    AppInstance = GetModuleHandle(NULL);
    WNDCLASSEX wndClass = {};
    wndClass.hInstance = AppInstance;
    wndClass.lpfnWndProc = WindowManager::WndProc;
    wndClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wndClass.hIconSm = wndClass.hIcon;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.lpszClassName = (LPCWSTR)AppName.CStr();
    wndClass.cbSize = sizeof(WNDCLASSEX);
    return RegisterClassEx(&wndClass);
}

void App::Finalize()
{
    if (FullScreen)
    {
        ChangeDisplaySettings(NULL, 0);
        FullScreen = false;
    }
    if (AppInstance)
    {
        UnregisterClass((LPCWSTR)AppName.CStr(), AppInstance);
        AppInstance = nullptr;
    }
}

void App::LoadData()
{
    pMainWindow->mouseInputCallback.Set(this, &App::PanelMouseInputHandler);
    pMainWindow->keyboardInputCallback.Set(this, &App::PanelKeyInputHandler);
    pSceneWindow->mouseInputCallback.Set(this, &App::SceneMouseInputHandler);
    pSceneWindow->keyboardInputCallback.Set(this, &App::SceneKeyInputHandler);

    Cube cube;
    Texture *pTexture = new Texture(String(IMAGE_FILE_PATH) + "michael-sum-unsplash.jpg");
    pTextures.Append(pTexture);
    cube.SetTexture(pTexture);
    mainScene->AddRenderable<Cube>(cube);
    mainScene->ApplyCamera(mainCamera);
    pSceneWindow->LoadScene(*mainScene);
    pMainWindow->Show();
}

void App::Update()
{
    WindowManager::GetSingleton().UpdateWindows();
}

void App::Render()
{
    WindowManager::GetSingleton().RenderWindows();
}

bool App::PanelKeyInputHandler(const KeyCombination &combination)
{
    return true;
}

bool App::PanelMouseInputHandler(const MouseInfo &mouseInfo)
{
    return true;
}

bool App::SceneKeyInputHandler(const KeyCombination &combination)
{
    if (combination.keys.Length() == 1)
    {
        auto keyPressed = combination.keys[0];
        switch (keyPressed)
        {
        case KeyCode::KeyCodeA: {
            const auto up = mainCamera->GetUpVector();
            const auto front = mainCamera->GetFrontVector();
            const auto left = up.Cross(front);
            mainCamera->Translate(left * 0.5f);
            return true;
        }
        case KeyCode::KeyCodeD: {
            const auto up = mainCamera->GetUpVector();
            const auto front = mainCamera->GetFrontVector();
            const auto left = up.Cross(front);
            mainCamera->Translate(-left * 0.5f);
            return true;
        }
        case KeyCode::KeyCodeW: {
            const auto front = mainCamera->GetFrontVector();
            mainCamera->Translate(front * 0.5f);
            return true;
        }
        case KeyCode::KeyCodeS: {
            const auto front = mainCamera->GetFrontVector();
            mainCamera->Translate(-front * 0.5f);
            return true;
        }
        default:
            break;
        }
        return false;
    }
}

bool App::SceneMouseInputHandler(const MouseInfo &mouseInfo)
{
    if (mouseInfo.leftButtonDown)
    {
        const auto &lastMousePos = mouseInfo.lastMousePosition;
        const auto &currentMousePos = mouseInfo.currentPosition;
        float xOffset = currentMousePos.x - lastMousePos.x;
        float yOffset = currentMousePos.y - lastMousePos.y;
        const float rotateAmount = 0.1f;
        xOffset *= rotateAmount;
        yOffset *= rotateAmount;
        mainCamera->Rotate(-yOffset, -xOffset);
        return true;
    }
    switch (mouseInfo.status)
    {
    case MouseStatus::LeftButtonDown: {
        break;
    }
    case MouseStatus::WheelRotated: {
        const auto frontVector = mainCamera->GetFrontVector();
        mainCamera->Translate(frontVector * mouseInfo.wheelDistance * 0.005f);
        return true;
    }
    default:
        break;
    }
    return false;
}

App::App()
    : pMainWindow(nullptr), pSceneWindow(nullptr), pTextures(),
      mainCamera(Point3D(1.5f, 1.5f, 1.5f), Point3D(-45.f, -135.f)),
      mainScene()
{
}

App::~App()
{
    for (size_t i = 0; i < pTextures.Length(); i++)
        delete pTextures[i];
}

int App::Execute()
{
    if (!Initialize())
        return EXIT_FAILURE;
    WindowInfo winInfo(String("Chia Engine"), false, WINDOW_WIDTH, WINDOW_HEIGHT);
    pMainWindow = WindowManager::GetSingleton().ConstructWindow(winInfo);
    if (!pMainWindow)
    {
        Finalize();
        return EXIT_FAILURE;
    }
    const auto sceneWindowWidth = WINDOW_WIDTH / 2;
    const auto sceneAreaHeight = WINDOW_HEIGHT - 50;
    WindowInfo childWndInfo(String(""), false, sceneWindowWidth, sceneAreaHeight, WINDOW_HEIGHT - sceneAreaHeight,
                            sceneWindowWidth / 2);
    pSceneWindow = WindowManager::GetSingleton().ConstructChildWindow(pMainWindow, childWndInfo);
    if (!pSceneWindow)
    {
        Finalize();
        return EXIT_FAILURE;
    }
    LoadData();

    MSG msg = {};
    bool shouldContinue = false;
    while (!shouldContinue)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Update();
        Render();
        shouldContinue = msg.message == WM_QUIT;
    }

    Finalize();
    return EXIT_SUCCESS;
}
