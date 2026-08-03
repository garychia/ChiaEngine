#ifndef CAMERA_CONTROLLER_HPP
#define CAMERA_CONTROLLER_HPP

#include "Data/Pointers.hpp"
#include "Display/Camera.hpp"
#include "SimInput.hpp"
#include "System/Module/EngineContext.hpp"
#include "System/Module/IModule.hpp"

// 確定性的相機控制器(Sim 層)。
//
// 相機是 Sim 狀態:FixedUpdate 讀 SimInput(context 服務),以固定步進
// 平移 / 旋轉相機 — 沒有牆鐘、沒有事件驅動,同樣的輸入序列必然產生
// 同樣的相機路徑,因此可以被 SimRecorder 錄下並重播。
//
// 輸入語意(由 CameraController 定義,View 層照此轉譯真實輸入):
//   actionBits: bit0 = 前進(W)  bit1 = 後退(S)
//               bit2 = 左移(A)  bit3 = 右移(D)
//   axisX / axisY: per-tick 滑鼠 delta(look);每 tick 讀完即消耗歸零。
//
// 注意:Reset() 用相對位移還原初始姿勢 — 相機由 SharedPtr 擁有(Scene 的
// WeakPtr 指向它),不能重建物件,否則 WeakPtr 會懸空。
class CameraController : public IModule
{
  public:
    static constexpr uint32_t BitMoveForward = 0x1u; // W
    static constexpr uint32_t BitMoveBack = 0x2u;    // S
    static constexpr uint32_t BitMoveLeft = 0x4u;    // A
    static constexpr uint32_t BitMoveRight = 0x8u;   // D

    CameraController(const Point3D &initialPosition = Point3D(1.5f, 1.5f, 1.5f),
                     const Point3D &initialRotation = Point3D(-45.f, -135.f))
        : pCamera(SharedPtr<Camera>::Construct(initialPosition, initialRotation)),
          initialPosition(initialPosition), initialRotation(initialRotation), pContext(nullptr)
    {
    }

    SharedPtr<Camera> GetCamera() const
    {
        return pCamera;
    }

    // 還原到初始姿勢(相對位移 — 見類別註解)
    void Reset()
    {
        pCamera->Translate(initialPosition - pCamera->GetPosition());
        pCamera->Rotate(initialRotation - pCamera->GetRotation());
    }

    void OnAttach(EngineContext &context) override
    {
        pContext = &context;
    }

    void FixedUpdate(const FrameClock &clock) override
    {
        SimInput *pInput = pContext ? pContext->ResolveService<SimInput>() : nullptr;
        if (!pInput)
            return;
        const float dt = static_cast<float>(clock.fixedDeltaSeconds);

        if (pInput->actionBits & BitMoveForward)
            pCamera->Translate(pCamera->GetFrontVector() * moveSpeed * dt);
        if (pInput->actionBits & BitMoveBack)
            pCamera->Translate(-pCamera->GetFrontVector() * moveSpeed * dt);
        if (pInput->actionBits & BitMoveLeft)
            pCamera->Translate(pCamera->GetUpVector().Cross(pCamera->GetFrontVector()) * moveSpeed * dt);
        if (pInput->actionBits & BitMoveRight)
            pCamera->Translate(-pCamera->GetUpVector().Cross(pCamera->GetFrontVector()) * moveSpeed * dt);

        // look:axis 是 per-tick delta,讀完消耗歸零(見類別註解)
        pCamera->Rotate(-pInput->axisY * lookSensitivity, pInput->axisX * lookSensitivity);
        pInput->axisX = 0.0f;
        pInput->axisY = 0.0f;
    }

  private:
    SharedPtr<Camera> pCamera;
    Point3D initialPosition;
    Point3D initialRotation;
    EngineContext *pContext;
    const float moveSpeed = 3.0f;        // 單位/秒
    const float lookSensitivity = 0.1f;  // 度/pixel
};

#endif // CAMERA_CONTROLLER_HPP
