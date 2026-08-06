#ifndef DISPLAY_FRAME_SERIALIZATION_TEST_HPP
#define DISPLAY_FRAME_SERIALIZATION_TEST_HPP

#include "Test.hpp"
#include "Display/Frame.hpp"
#include "Display/Color.hpp"
#include "Geometry/3D/Point3D.hpp"

// Frame 序列化測試(無頭,值語意):
// 驗收 1 & 2 — 相同場景 → 相同命令流字串(確定性);新命令 append / serialize → deserialize round-trip。
// 只依賴值語意命令,不含 legacy 指標命令。仍須連結 Display 的純資料 .cpp
// (Camera/GUILayout/…),因為 Frame::Serialize 對 legacy 命令會取語意內容 —
// 皆為無 GPU 依賴的純 C++ 檔,故仍可 standalone headless 執行。
class FrameSerializationTest : public Test
{
  private:
    static Frame BuildValueScene()
    {
        Frame frame;
        frame.BeginFrame();
        frame.PushTransform(Point3D(1, 2, 3), Point3D(0, 45, 0), Point3D(1, 2, 1));
        frame.BindMaterial(0xABCDEF123456ULL);
        frame.DrawMesh(0x1234ULL);
        frame.SetViewport(10, 20, 800, 600);
        frame.DrawText(7, String(u"Hello World"), 16.0f, Color(0.25f, 0.5f, 0.75f, 1));
        frame.PopTransform();
        frame.EndFrame();
        return frame;
    }

  public:
    FrameSerializationTest() : Test("DisplayFrameSerialization")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("Serialize determinism (same scene -> same stream)");
        {
            Frame a = BuildValueScene();
            Frame b = BuildValueScene();
            EXPECT_TRUE(a.Serialize() == b.Serialize(), "相同場景 → 相同命令流字串.", true);
            EXPECT_TRUE(a.GetNumCommands() == 8, "命令數正確(8 條).", true);
        }

        TEST_MESSAGE("Serialize round-trip (new commands)");
        {
            Frame a = BuildValueScene();
            const String serialized = a.Serialize();
            Frame b = Frame::Deserialize(serialized);
            EXPECT_TRUE(b.Serialize() == serialized, "序列化 → 反序列化 → 再序列化一致.", true);
            EXPECT_TRUE(b.GetNumCommands() == a.GetNumCommands(), "命令數一致.", true);
        }

        TEST_MESSAGE("Round-trip payload values");
        {
            Frame b = Frame::Deserialize(FrameSerializationTest::BuildValueScene().Serialize());

            const Frame::CommandData &push = b.GetCommand(1);
            EXPECT_TRUE(push.command == Frame::Command::PushTransform, "命令 1 為 PushTransform.", true);
            EXPECT_TRUE(push.transform.position.x == 1.0f && push.transform.position.y == 2.0f && push.transform.position.z == 3.0f,
                        "PushTransform position 保留.", true);
            EXPECT_TRUE(push.transform.rotation.y == 45.0f, "PushTransform rotation 保留.", true);
            EXPECT_TRUE(push.transform.scale.x == 1.0f && push.transform.scale.y == 2.0f, "PushTransform scale 保留.", true);

            const Frame::CommandData &material = b.GetCommand(2);
            EXPECT_TRUE(material.command == Frame::Command::BindMaterial && material.materialId == 0xABCDEF123456ULL,
                        "BindMaterial 保留 materialId.", true);

            const Frame::CommandData &mesh = b.GetCommand(3);
            EXPECT_TRUE(mesh.command == Frame::Command::DrawMesh && mesh.meshId == 0x1234ULL,
                        "DrawMesh 保留 meshId.", true);

            const Frame::CommandData &viewport = b.GetCommand(4);
            EXPECT_TRUE(viewport.command == Frame::Command::SetViewport && viewport.viewport.width == 800.0f &&
                            viewport.viewport.height == 600.0f,
                        "SetViewport 保留尺寸.", true);

            const Frame::CommandData &text = b.GetCommand(5);
            EXPECT_TRUE(text.command == Frame::Command::DrawText, "命令 5 為 DrawText.", true);
            EXPECT_TRUE(text.fontId == 7 && text.textSize == 16.0f, "DrawText 保留 fontId/size.", true);
            EXPECT_TRUE(text.text == String(u"Hello World"), "DrawText 保留 text.", true);
            EXPECT_TRUE(text.textColor.R == 0.25f && text.textColor.G == 0.5f && text.textColor.B == 0.75f,
                        "DrawText 保留 color.", true);

            const Frame::CommandData &pop = b.GetCommand(6);
            EXPECT_TRUE(pop.command == Frame::Command::PopTransform, "命令 6 為 PopTransform.", true);
        }

        SUCCESS_MESSAGE("DisplayFrameSerialization");
        return true;
    }
};

#endif // DISPLAY_FRAME_SERIALIZATION_TEST_HPP