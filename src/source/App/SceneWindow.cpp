#include "App/SceneWindow.hpp"
#include "Paths.hpp"
#include "Geometry/Primitives.hpp"

SceneWindow::SceneWindow(const WindowInfo &info) : Window(info), pTextures()
{
    pMainCamera = SharedPtr<Camera>::Construct(Point3D(1.5f, 1.5f, 1.5f), Point3D(-45.f, -135.f));
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
    pMainScene->ApplyCamera(pMainCamera);
    return LoadScene(*pMainScene);
}

bool SceneWindow::OnKeyboardInputReceived(const KeyCombination &combination)
{
    if (combination.keys.Length() == 1)
    {
        auto keyPressed = combination.keys[0];
        switch (keyPressed)
        {
        case KeyCode::KeyCodeA: {
            const auto up = pMainCamera->GetUpVector();
            const auto front = pMainCamera->GetFrontVector();
            const auto left = up.Cross(front);
            pMainCamera->Translate(left * 0.5f);
            return true;
        }
        case KeyCode::KeyCodeD: {
            const auto up = pMainCamera->GetUpVector();
            const auto front = pMainCamera->GetFrontVector();
            const auto left = up.Cross(front);
            pMainCamera->Translate(-left * 0.5f);
            return true;
        }
        case KeyCode::KeyCodeW: {
            const auto front = pMainCamera->GetFrontVector();
            pMainCamera->Translate(front * 0.5f);
            return true;
        }
        case KeyCode::KeyCodeS: {
            const auto front = pMainCamera->GetFrontVector();
            pMainCamera->Translate(-front * 0.5f);
            return true;
        }
        default:
            break;
        }
    }
    return false;
}

bool SceneWindow::OnMouseInputReceived(const MouseInfo &mouseInfo)
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
        pMainCamera->Rotate(-yOffset, -xOffset);
        return true;
    }
    switch (mouseInfo.status)
    {
    case MouseStatus::LeftButtonDown: {
        break;
    }
    case MouseStatus::WheelRotated: {
        const auto frontVector = pMainCamera->GetFrontVector();
        pMainCamera->Translate(frontVector * mouseInfo.wheelDistance * 0.005f);
        return true;
    }
    default:
        break;
    }
    return false;
}
