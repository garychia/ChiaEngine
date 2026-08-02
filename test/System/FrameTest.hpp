#ifndef DISPLAY_FRAME_TEST_HPP
#define DISPLAY_FRAME_TEST_HPP

#include "Test.hpp"
#include "Display/Frame.hpp"
#include "Display/Camera.hpp"
#include "Display/IRenderable.hpp"
#include "Display/GUI/GUILayout.hpp"

// Frame 命令錄製測試(無頭):
// 需要 Display 的 .cpp 一起連結,用 standalone 編譯:
//   g++ -std=c++17 -I test -I test/System -I src/include frame_standalone.cpp
//       src/source/Display/IRenderable.cpp src/source/Display/Camera.cpp
//       src/source/Display/Color.cpp src/source/Display/Texture.cpp
//       src/source/Display/Shader.cpp src/source/Display/GUI/GUILayout.cpp
//       src/source/Display/GUI/GUILayer.cpp src/source/Geometry/2D/Point2D.cpp
//       src/source/Geometry/3D/Point3D.cpp src/source/Math/Math.cpp -o /tmp/FrameTest
class FrameTest : public Test
{
  public:
    FrameTest() : Test("DisplayFrame")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("Frame command recording");
        {
            Frame frame;
            EXPECT_TRUE(frame.GetNumCommands() == 0, "初始無命令.", true);

            IRenderable renderable;
            SharedPtr<Camera> pCamera = SharedPtr<Camera>::Construct();
            WeakPtr<Camera> weakCamera(pCamera);
            GUILayout layout;

            frame.BeginFrame();
            frame.SetCamera(weakCamera);
            frame.DrawRenderable(renderable);
            frame.DrawGUILayout(layout);
            frame.EndFrame();

            EXPECT_TRUE(frame.GetNumCommands() == 5, "5 條命令.", true);
            EXPECT_TRUE(frame.GetCommand(0).command == Frame::Command::BeginFrame, "順序: BeginFrame.", true);
            EXPECT_TRUE(frame.GetCommand(1).command == Frame::Command::SetCamera, "順序: SetCamera.", true);
            EXPECT_TRUE(frame.GetCommand(2).command == Frame::Command::DrawRenderable, "順序: DrawRenderable.", true);
            EXPECT_TRUE(frame.GetCommand(3).command == Frame::Command::DrawGUILayout, "順序: DrawGUILayout.", true);
            EXPECT_TRUE(frame.GetCommand(4).command == Frame::Command::EndFrame, "順序: EndFrame.", true);
        }

        TEST_MESSAGE("Frame payloads");
        {
            Frame frame;
            IRenderable renderable;
            SharedPtr<Camera> pCamera = SharedPtr<Camera>::Construct();
            GUILayout layout;

            frame.SetCamera(WeakPtr<Camera>(pCamera));
            frame.DrawRenderable(renderable);
            frame.DrawGUILayout(layout);

            EXPECT_TRUE(frame.GetCommand(0).pCamera == pCamera.operator->(), "相機指標正確.", true);
            EXPECT_TRUE(frame.GetCommand(0).pRenderable == nullptr, "未使用欄位為 nullptr.", true);
            EXPECT_TRUE(frame.GetCommand(1).pRenderable == &renderable, "renderable 指標正確.", true);
            EXPECT_TRUE(frame.GetCommand(1).pCamera == nullptr, "未使用欄位為 nullptr.", true);
            EXPECT_TRUE(frame.GetCommand(2).pLayout == &layout, "layout 指標正確.", true);
        }

        TEST_MESSAGE("Frame invalid camera is nullptr");
        {
            Frame frame;
            frame.SetCamera(WeakPtr<Camera>()); // 無效相機
            EXPECT_TRUE(frame.GetCommand(0).pCamera == nullptr, "無效相機 → nullptr(安全).", true);
        }

        TEST_MESSAGE("Frame clear and reuse");
        {
            Frame frame;
            frame.BeginFrame();
            frame.EndFrame();
            EXPECT_TRUE(frame.GetNumCommands() == 2, "錄製兩條.", true);
            frame.Clear();
            EXPECT_TRUE(frame.GetNumCommands() == 0, "Clear 後歸零.", true);
            frame.BeginFrame(); // 重複使用
            frame.EndFrame();
            EXPECT_TRUE(frame.GetNumCommands() == 2, "Clear 後可重複錄製.", true);
        }

        SUCCESS_MESSAGE("DisplayFrame");
        return true;
    }
};

#endif // DISPLAY_FRAME_TEST_HPP
