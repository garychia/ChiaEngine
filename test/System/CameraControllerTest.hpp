#ifndef SYSTEM_CAMERA_CONTROLLER_TEST_HPP
#define SYSTEM_CAMERA_CONTROLLER_TEST_HPP

#include "Test.hpp"
#include "System/Module/CameraController.hpp"
#include "System/Module/Engine.hpp"
#include "System/Module/IModule.hpp"
#include "System/Module/SimInput.hpp"
#include "System/Module/SimRecorder.hpp"

// ---------------- 測試主體 ----------------

class CameraControllerTest : public Test
{
  public:
    CameraControllerTest() : Test("SystemCameraController")
    {
    }

    bool Run() noexcept override
    {
        // 腳本化輸入:60 tick(1 秒 @60Hz),涵蓋 WASD 組合、look 軸、暫停
        static const SimInput script[60] = {
            // 前 10 tick:按住 W(前進)
            {CameraController::BitMoveForward, 0.0f, 0.0f},
            {CameraController::BitMoveForward, 0.0f, 0.0f},
            {CameraController::BitMoveForward, 0.0f, 0.0f},
            {CameraController::BitMoveForward, 0.0f, 0.0f},
            {CameraController::BitMoveForward, 0.0f, 0.0f},
            {CameraController::BitMoveForward, 0.0f, 0.0f},
            {CameraController::BitMoveForward, 0.0f, 0.0f},
            {CameraController::BitMoveForward, 0.0f, 0.0f},
            {CameraController::BitMoveForward, 0.0f, 0.0f},
            {CameraController::BitMoveForward, 0.0f, 0.0f},
            // 10 tick:W + A + look 右
            {CameraController::BitMoveForward | CameraController::BitMoveLeft, 5.0f, 0.0f},
            {CameraController::BitMoveForward | CameraController::BitMoveLeft, 5.0f, 0.0f},
            {CameraController::BitMoveForward | CameraController::BitMoveLeft, 5.0f, 0.0f},
            {CameraController::BitMoveForward | CameraController::BitMoveLeft, 5.0f, 0.0f},
            {CameraController::BitMoveForward | CameraController::BitMoveLeft, 5.0f, 0.0f},
            {CameraController::BitMoveForward | CameraController::BitMoveLeft, 5.0f, 0.0f},
            {CameraController::BitMoveForward | CameraController::BitMoveLeft, 5.0f, 0.0f},
            {CameraController::BitMoveForward | CameraController::BitMoveLeft, 5.0f, 0.0f},
            {CameraController::BitMoveForward | CameraController::BitMoveLeft, 5.0f, 0.0f},
            {CameraController::BitMoveForward | CameraController::BitMoveLeft, 5.0f, 0.0f},
            // 10 tick:暫停(零輸入)
            {0x0u, 0.0f, 0.0f}, {0x0u, 0.0f, 0.0f}, {0x0u, 0.0f, 0.0f},
            {0x0u, 0.0f, 0.0f}, {0x0u, 0.0f, 0.0f}, {0x0u, 0.0f, 0.0f},
            {0x0u, 0.0f, 0.0f}, {0x0u, 0.0f, 0.0f}, {0x0u, 0.0f, 0.0f},
            {0x0u, 0.0f, 0.0f},
            // 10 tick:look 上下 + D
            {CameraController::BitMoveRight, 0.0f, -3.0f},
            {CameraController::BitMoveRight, 0.0f, -3.0f},
            {CameraController::BitMoveRight, 0.0f, -3.0f},
            {CameraController::BitMoveRight, 0.0f, -3.0f},
            {CameraController::BitMoveRight, 0.0f, -3.0f},
            {CameraController::BitMoveRight, 0.0f, -3.0f},
            {CameraController::BitMoveRight, 0.0f, -3.0f},
            {CameraController::BitMoveRight, 0.0f, -3.0f},
            {CameraController::BitMoveRight, 0.0f, -3.0f},
            {CameraController::BitMoveRight, 0.0f, -3.0f},
            // 10 tick:S + look 左
            {CameraController::BitMoveBack, -4.0f, 2.0f},
            {CameraController::BitMoveBack, -4.0f, 2.0f},
            {CameraController::BitMoveBack, -4.0f, 2.0f},
            {CameraController::BitMoveBack, -4.0f, 2.0f},
            {CameraController::BitMoveBack, -4.0f, 2.0f},
            {CameraController::BitMoveBack, -4.0f, 2.0f},
            {CameraController::BitMoveBack, -4.0f, 2.0f},
            {CameraController::BitMoveBack, -4.0f, 2.0f},
            {CameraController::BitMoveBack, -4.0f, 2.0f},
            {CameraController::BitMoveBack, -4.0f, 2.0f},
            // 最後 10 tick:全零(確認零輸入狀態穩定)
            {0x0u, 0.0f, 0.0f}, {0x0u, 0.0f, 0.0f}, {0x0u, 0.0f, 0.0f},
            {0x0u, 0.0f, 0.0f}, {0x0u, 0.0f, 0.0f}, {0x0u, 0.0f, 0.0f},
            {0x0u, 0.0f, 0.0f}, {0x0u, 0.0f, 0.0f}, {0x0u, 0.0f, 0.0f},
            {0x0u, 0.0f, 0.0f},
        };
        const int N = 60;

        TEST_MESSAGE("Record → replay → per-tick camera path identical");
        {
            // ---- live run ----
            Engine engine(1);
            SimRecorder recorder;
            CameraController controller;
            engine.Attach(&recorder); // 先附著:輸入先寫,系統後讀
            engine.Attach(&controller);

            struct CamState
            {
                Point3D pos, rot;
            };
            DynamicArray<CamState> livePath;
            for (int i = 0; i < N; i++)
            {
                recorder.GetLiveInput() = script[i];
                engine.Tick(1.0 / 60.0);
                CamState s;
                s.pos = controller.GetCamera()->GetPosition();
                s.rot = controller.GetCamera()->GetRotation();
                livePath.Append(s);
            }
            EXPECT_TRUE(recorder.GetInputCount() == static_cast<size_t>(N), "錄到 N 筆輸入.", true);

            // ---- replay run ----
            Engine replayEngine(1);
            SimRecorder replayRecorder;
            CameraController replayController;
            replayRecorder.SetReplaying(true);
            replayRecorder.LoadLog(recorder.GetLog());
            replayEngine.Attach(&replayRecorder);
            replayEngine.Attach(&replayController);

            bool allEqual = true;
            for (int i = 0; i < N; i++)
            {
                replayEngine.Tick(1.0 / 60.0); // 不寫輸入 — recorder 自己餵
                const Point3D &p = replayController.GetCamera()->GetPosition();
                const Point3D &r = replayController.GetCamera()->GetRotation();
                const CamState &expected = livePath[static_cast<size_t>(i)];
                if (p.x != expected.pos.x || p.y != expected.pos.y || p.z != expected.pos.z ||
                    r.x != expected.rot.x || r.y != expected.rot.y || r.z != expected.rot.z)
                    allEqual = false;
            }
            EXPECT_TRUE(allEqual, "每 tick 相機位置/旋轉與 live bit 級相同(確定性 replay).", true);
        }

        TEST_MESSAGE("Tampered input → camera path diverges");
        {
            Engine engine(1);
            SimRecorder recorder;
            CameraController controller;
            engine.Attach(&recorder);
            engine.Attach(&controller);

            for (int i = 0; i < N; i++)
            {
                recorder.GetLiveInput() = script[i];
                engine.Tick(1.0 / 60.0);
            }
            const Point3D liveFinalPos = controller.GetCamera()->GetPosition();

            // 篡改錄音:第 15 tick 的 bit0(前進)清掉
            DynamicArray<SimInput> tampered = recorder.GetLog();
            tampered[15].actionBits &= ~CameraController::BitMoveForward;

            Engine replayEngine(1);
            SimRecorder replayRecorder;
            CameraController replayController;
            replayRecorder.SetReplaying(true);
            replayRecorder.LoadLog(tampered);
            replayEngine.Attach(&replayRecorder);
            replayEngine.Attach(&replayController);

            for (int i = 0; i < N; i++)
                replayEngine.Tick(1.0 / 60.0);

            const Point3D &p = replayController.GetCamera()->GetPosition();
            const bool diverged = (p.x != liveFinalPos.x || p.y != liveFinalPos.y || p.z != liveFinalPos.z);
            EXPECT_TRUE(diverged, "篡改輸入 → 最終相機位置分歧(證明 replay 吃錄音).", true);
        }

        TEST_MESSAGE("Reset restores initial pose");
        {
            Engine engine(1);
            SimRecorder recorder;
            CameraController controller(Point3D(1.5f, 1.5f, 1.5f), Point3D(-45.f, -135.f));
            engine.Attach(&recorder);
            engine.Attach(&controller);

            for (int i = 0; i < N; i++)
            {
                recorder.GetLiveInput() = script[i];
                engine.Tick(1.0 / 60.0);
            }
            controller.Reset();

            const Point3D &p = controller.GetCamera()->GetPosition();
            const Point3D &r = controller.GetCamera()->GetRotation();
            EXPECT_TRUE(p.x == 1.5f && p.y == 1.5f && p.z == 1.5f, "Reset 後位置回到初始.", true);
            EXPECT_TRUE(r.x == -45.f && r.y == -135.f && r.z == 0.f, "Reset 後旋轉回到初始.", true);
        }

        SUCCESS_MESSAGE("SystemCameraController");
        return true;
    }
};

#endif // SYSTEM_CAMERA_CONTROLLER_TEST_HPP
