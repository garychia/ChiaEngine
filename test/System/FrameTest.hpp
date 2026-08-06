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

        TEST_MESSAGE("New value commands append");
        {
            Frame frame;
            frame.BeginFrame();
            frame.PushTransform(Point3D(1, 2, 3), Point3D(0, 45, 0), Point3D(1, 2, 1));
            frame.BindMaterial(0xABCULL);
            frame.DrawMesh(0x1234ULL);
            frame.SetViewport(0, 0, 1920, 1080);
            frame.DrawText(7, String(u"Hello"), 16.0f, Color(1, 0.5f, 0.25f, 1));
            frame.PopTransform();
            frame.EndFrame();
            EXPECT_TRUE(frame.GetNumCommands() == 8, "8 條命令(5 保留 + 6 條新增中的 6:Begin/Push/Bind/DrawMesh/Viewport/Text/Pop/End = 8).", true);
            EXPECT_TRUE(frame.GetCommand(1).command == Frame::Command::PushTransform, "PushTransform.", true);
            EXPECT_TRUE(frame.GetCommand(1).transform.position.x == 1 && frame.GetCommand(1).transform.position.y == 2 &&
                            frame.GetCommand(1).transform.position.z == 3,
                        "position.", true);
        }

        TEST_MESSAGE("Frame serialize determinism (same scene -> same stream)");
        {
            Frame a, b;
            a.BeginFrame();
            a.PushTransform(Point3D(1, 2, 3), Point3D(0, 45, 0), Point3D(1, 2, 1));
            a.BindMaterial(1234567ULL);
            a.DrawMesh(99ULL);
            a.SetViewport(0, 0, 1920, 1080);
            a.DrawText(7, String(u"Hello World"), 16.0f, Color(1, 0.5f, 0.25f, 1));
            a.PopTransform();
            a.EndFrame();
            b.BeginFrame();
            b.PushTransform(Point3D(1, 2, 3), Point3D(0, 45, 0), Point3D(1, 2, 1));
            b.BindMaterial(1234567ULL);
            b.DrawMesh(99ULL);
            b.SetViewport(0, 0, 1920, 1080);
            b.DrawText(7, String(u"Hello World"), 16.0f, Color(1, 0.5f, 0.25f, 1));
            b.PopTransform();
            b.EndFrame();
            EXPECT_TRUE(a.Serialize() == b.Serialize(), "相同場景 → 相同命令流字串.", true);
        }

        TEST_MESSAGE("Frame serialize round-trip (new commands)");
        {
            Frame a;
            a.BeginFrame();
            a.PushTransform(Point3D(1, 2, 3), Point3D(0, 45, 0), Point3D(1, 2, 1));
            a.BindMaterial(0xABCDEF123456ULL);
            a.DrawMesh(0x1234ULL);
            a.SetViewport(10, 20, 800, 600);
            a.DrawText(7, String(u"Hello World"), 16.0f, Color(0.25f, 0.5f, 0.75f, 1));
            a.PopTransform();
            a.EndFrame();

            Frame b = Frame::Deserialize(a.Serialize());
            EXPECT_TRUE(a.Serialize() == b.Serialize(), "序列化 → 反序列化 → 再序列化一致.", true);
            EXPECT_TRUE(b.GetNumCommands() == a.GetNumCommands(), "命令數一致.", true);
            const Frame::CommandData &text = b.GetCommand(5);
            EXPECT_TRUE(text.command == Frame::Command::DrawText && text.fontId == 7, "DrawText 重建 fontId.", true);
            EXPECT_TRUE(text.text == String(u"Hello World"), "DrawText 重建 text.", true);
            EXPECT_TRUE(text.textSize == 16.0f, "DrawText 重建 size.", true);
            EXPECT_TRUE(text.textColor.R == 0.25f && text.textColor.G == 0.5f && text.textColor.B == 0.75f,
                        "DrawText 重建 color.", true);
        }

        SUCCESS_MESSAGE("DisplayFrame");
        return true;
    }
};

#endif // DISPLAY_FRAME_TEST_HPP
