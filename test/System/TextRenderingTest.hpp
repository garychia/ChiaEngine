#ifndef TEXT_RENDERING_TEST_HPP
#define TEXT_RENDERING_TEST_HPP

#include "Test.hpp"

#include "Display/Frame.hpp"
#include "Display/GUI/Button.hpp"
#include "Display/GUI/GUIFrameProjector.hpp"
#include "Display/GUI/GUILayer.hpp"
#include "Display/GUI/GUILayout.hpp"
#include "Display/Text/GlyphAtlas.hpp"
#include "Display/Text/TextLayout.hpp"

// P7e 文字渲染測試(無頭,驗收 1/2/3):
//  - 驗收 1:DrawText 正確的 quad/UV 序列 → TextLayout golden + 序列化確定性。
//  - 驗收 2:GUI 按鈕 label → GUIFrameProjector 把 label 投影成 Frame 的 DrawText 命令。
//  - 驗收 3:ASCII 基本集、換行、截斷、非 ASCII fallback。
// 全程不依賴 GPU;只需連結純資料 .cpp(GlyphAtlas/TextLayout/GUIFrameProjector/Button/…)。

class TextRenderingTest : public Test
{
  public:
    TextRenderingTest() : Test("TextRendering")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("GlyphAtlas structure");
        {
            const GlyphAtlas &atlas = GlyphAtlas::GetDefault();
            EXPECT_TRUE(atlas.GetAtlasWidth() == 143, "圖集寬 143(16 格 × 8px + padding).", true);
            EXPECT_TRUE(atlas.GetAtlasHeight() == 71, "圖集高 71(8 列 × 8px + padding).", true);
            EXPECT_TRUE(atlas.GetRGBA() != nullptr, "RGBA 資料存在.", true);

            const GlyphMetrics space = atlas.GetMetrics((char16_t)' ');
            const GlyphMetrics a = atlas.GetMetrics((char16_t)'A');
            EXPECT_TRUE(a.advance == 8.0f, "advance = 8 字型單位.", true);
            EXPECT_TRUE(a.u0 >= 0.0f && a.u0 < a.u1 && a.u1 <= 1.0f, "U 範圍 [0,1] 且 u0 < u1.", true);
            EXPECT_TRUE(a.v0 >= 0.0f && a.v0 < a.v1 && a.v1 <= 1.0f, "V 範圍 [0,1] 且 v0 < v1.", true);
            EXPECT_TRUE(a.u0 != space.u0 || a.v0 != space.v0, "'A' 與空格在不同格.", true);
        }

        TEST_MESSAGE("GlyphAtlas fallback + determinism");
        {
            const GlyphAtlas &atlas = GlyphAtlas::GetDefault();
            const GlyphMetrics fallback = atlas.GetMetrics((char16_t)0x4E2D); // '中' 非 ASCII
            const GlyphMetrics space = atlas.GetMetrics((char16_t)' ');
            EXPECT_TRUE(fallback.u0 == space.u0 && fallback.v0 == space.v0 && fallback.u1 == space.u1 &&
                            fallback.v1 == space.v1,
                        "非 ASCII → 空白 fallback.", true);
            EXPECT_TRUE(GlyphAtlas::DefaultFontId() == GlyphAtlas::DefaultFontId(), "fontId 內容定址且確定.", true);
            EXPECT_TRUE(GlyphAtlas::DefaultFontId() != 0, "fontId 非零.", true);
        }

        TEST_MESSAGE("TextLayout determinism");
        {
            DynamicArray<TextQuad> a, b;
            TextLayout::Layout(GlyphAtlas::GetDefault(), u"HELLO WORLD", 16.0f, 0.0f, a);
            TextLayout::Layout(GlyphAtlas::GetDefault(), u"HELLO WORLD", 16.0f, 0.0f, b);
            EXPECT_TRUE(a.GetNElements() == b.GetNElements(), "相同文字 → 相同 quad 數.", true);
            EXPECT_TRUE(TextLayout::SerializeQuads(a) == TextLayout::SerializeQuads(b), "序列化確定性.", true);
        }

        TEST_MESSAGE("TextLayout HELLO golden (advance + UV)");
        {
            const GlyphAtlas &atlas = GlyphAtlas::GetDefault();
            DynamicArray<TextQuad> quads;
            TextLayout::Layout(atlas, u"HELLO", 16.0f, 0.0f, quads);
            EXPECT_TRUE(quads.GetNElements() == 5, "5 字元 → 5 quad.", true);
            const char16_t word[] = u"HELLO";
            for (size_t i = 0; i < quads.GetNElements(); i++)
            {
                const TextQuad &q = quads[i];
                EXPECT_TRUE(q.x0 == static_cast<float>(i) * 16.0f && q.x1 == static_cast<float>(i + 1) * 16.0f,
                            "advance 累加正確(每字元 16px).", true);
                EXPECT_TRUE(q.y0 == 0.0f && q.y1 == 16.0f, "單行 y 範圍正確.", true);
                const GlyphMetrics m = atlas.GetMetrics(word[i]);
                EXPECT_TRUE(q.u0 == m.u0 && q.v0 == m.v0 && q.u1 == m.u1 && q.v1 == m.v1,
                            "UV 與圖集對應字形一致.", true);
            }
        }

        TEST_MESSAGE("TextLayout newline");
        {
            DynamicArray<TextQuad> quads;
            TextLayout::Layout(GlyphAtlas::GetDefault(), u"AB\nCD", 16.0f, 0.0f, quads);
            EXPECT_TRUE(quads.GetNElements() == 4, "換行 → 4 quad.", true);
            EXPECT_TRUE(quads[0].y0 == 0.0f && quads[1].y0 == 0.0f, "第一行 y = 0.", true);
            EXPECT_TRUE(quads[2].y0 == 19.2f && quads[3].y0 == 19.2f, "第二行 y = size*1.2.", true);
            EXPECT_TRUE(quads[2].x0 == 0.0f, "換行後 x 歸零.", true);
        }

        TEST_MESSAGE("TextLayout truncation");
        {
            const GlyphAtlas &atlas = GlyphAtlas::GetDefault();
            DynamicArray<TextQuad> quads;
            TextLayout::Layout(atlas, u"HELLO WORLD", 16.0f, 0.0f, quads); // 不截斷
            EXPECT_TRUE(quads.GetNElements() == 11, "maxWidth=0 → 不截斷.", true);

            TextLayout::Layout(atlas, u"HELLO WORLD", 16.0f, 160.0f, quads); // 10 字元寬
            EXPECT_TRUE(quads.GetNElements() == 10, "超寬 → 截斷成 HELLO W + '...'.", true);
            EXPECT_TRUE(quads[9].x1 == 160.0f, "截斷總寬不超過 maxWidth.", true);
            const GlyphMetrics dot = atlas.GetMetrics((char16_t)'.');
            for (size_t i = 7; i < 10; i++)
                EXPECT_TRUE(quads[i].u0 == dot.u0 && quads[i].u1 == dot.u1, "結尾三字元為 '.'.", true);
        }

        TEST_MESSAGE("TextLayout non-ASCII fallback");
        {
            DynamicArray<TextQuad> quads;
            TextLayout::Layout(GlyphAtlas::GetDefault(), u"A\u4E2D", 16.0f, 0.0f, quads); // "A中"
            EXPECT_TRUE(quads.GetNElements() == 2, "非 ASCII 不崩潰、照樣產出 quad.", true);
            const GlyphMetrics space = GlyphAtlas::GetDefault().GetMetrics((char16_t)' ');
            EXPECT_TRUE(quads[1].u0 == space.u0 && quads[1].v0 == space.v0, "非 ASCII → 空白 UV.", true);
        }

        TEST_MESSAGE("GUIFrameProjector emits DrawText");
        {
            GUILayout layout;
            auto pLayer = SharedPtr<GUILayer>::Construct(Point2D(800, 600), Border(0.f, 0.f, 800.f, 600.f));
            layout.AddLayer(pLayer);
            auto pButton = pLayer->AddComponent<Button>(Point2D(800, 600), Border(0.f, 0.f, 90.f, 40.f));
            pButton->SetLabel(String(u"File"));
            pButton->SetFontSize(14.0f);
            pButton->SetTextColor(Color(0.9f, 0.9f, 0.2f, 1.0f));

            Frame frame;
            frame.BeginFrame();
            frame.DrawGUILayout(layout);
            GUIFrameProjector::ProjectLabels(layout, GlyphAtlas::GetDefault(), GlyphAtlas::DefaultFontId(), frame);
            frame.EndFrame();

            EXPECT_TRUE(frame.GetNumCommands() == 6, "Begin/DrawGUILayout/Push/DrawText/Pop/End = 6.", true);
            EXPECT_TRUE(frame.GetCommand(2).command == Frame::Command::PushTransform, "命令 2 為 PushTransform.", true);
            EXPECT_TRUE(frame.GetCommand(3).command == Frame::Command::DrawText, "命令 3 為 DrawText.", true);
            const Frame::CommandData &text = frame.GetCommand(3);
            EXPECT_TRUE(text.fontId == GlyphAtlas::DefaultFontId(), "DrawText fontId 正確.", true);
            EXPECT_TRUE(text.text == String(u"File"), "DrawText text 正確.", true);
            EXPECT_TRUE(text.textSize == 14.0f, "DrawText textSize 正確.", true);
            EXPECT_TRUE(text.textColor.R == 0.9f && text.textColor.G == 0.9f && text.textColor.B == 0.2f,
                        "DrawText textColor 正確.", true);
            EXPECT_TRUE(frame.GetCommand(4).command == Frame::Command::PopTransform, "命令 4 為 PopTransform.", true);
            EXPECT_TRUE(frame.Serialize().Length() > 0, "命令流可序列化.", true);
        }

        TEST_MESSAGE("GUIFrameProjector skips empty labels");
        {
            GUILayout layout;
            auto pLayer = SharedPtr<GUILayer>::Construct(Point2D(800, 600), Border(0.f, 0.f, 800.f, 600.f));
            layout.AddLayer(pLayer);
            pLayer->AddComponent<Button>(Point2D(800, 600), Border(0.f, 0.f, 90.f, 40.f)); // 無 label

            Frame frame;
            frame.BeginFrame();
            frame.DrawGUILayout(layout);
            GUIFrameProjector::ProjectLabels(layout, GlyphAtlas::GetDefault(), GlyphAtlas::DefaultFontId(), frame);
            frame.EndFrame();
            EXPECT_TRUE(frame.GetNumCommands() == 3, "無 label → 只發 Begin/DrawGUILayout/End.", true);
        }

        SUCCESS_MESSAGE("TextRendering");
        return true;
    }
};

#endif // TEXT_RENDERING_TEST_HPP
