#include "Display/Text/TextLayout.hpp"

#include "Data/Str.hpp"

namespace
{
constexpr float kFontUnitsPerGlyph = 8.0f; // 與 GlyphAtlas 一致:一格 = 8x8 字型單位
constexpr float kLineHeightScale = 1.2f;   // lineHeight = size * 1.2
} // namespace

void TextLayout::Layout(const GlyphAtlas &atlas, const char16_t *text, float size,
                        float maxWidthPx, DynamicArray<TextQuad> &outQuads)
{
    outQuads.RemoveAll();
    if (text == nullptr || size <= 0.0f)
        return;

    const float scale = size / kFontUnitsPerGlyph;
    const float lineHeight = size * kLineHeightScale;
    const float ellipsisWidth = 3.0f * size; // 三個 '.' 的寬(等寬字型)

    bool truncated = false;

    float x = 0.0f;
    float y = 0.0f;

    for (size_t i = 0; text[i] != 0; i++)
    {
        const char16_t ch = text[i];
        if (ch == (char16_t)'\n')
        {
            x = 0.0f;
            y += lineHeight;
            continue;
        }

        const GlyphMetrics metrics = atlas.GetMetrics(ch);
        const float advancePx = metrics.advance * scale;

        // 截斷:下一字元超寬 → 退到省略號放得下,再補 "...",整個佈局結束。
        if (maxWidthPx > 0.0f && x + advancePx > maxWidthPx)
        {
            while (!outQuads.IsEmpty() && x + ellipsisWidth > maxWidthPx)
            {
                const TextQuad &last = outQuads.GetLast();
                x -= (last.x1 - last.x0);
                outQuads.RemoveLast();
            }
            if (x + ellipsisWidth <= maxWidthPx)
            {
                const GlyphMetrics dot = atlas.GetMetrics((char16_t)'.');
                for (int k = 0; k < 3; k++)
                {
                    TextQuad q;
                    q.x0 = x;
                    q.y0 = y;
                    q.x1 = x + size;
                    q.y1 = y + size;
                    q.u0 = dot.u0;
                    q.v0 = dot.v0;
                    q.u1 = dot.u1;
                    q.v1 = dot.v1;
                    outQuads.Append(q);
                    x += size;
                }
            }
            truncated = true;
            break;
        }
        // 空格/空白 fallback 也產出 quad(透明),維持「字形數 = 字元數」的直覺。
        TextQuad quad;
        quad.x0 = x;
        quad.y0 = y;
        quad.x1 = x + size;
        quad.y1 = y + size;
        quad.u0 = metrics.u0;
        quad.v0 = metrics.v0;
        quad.u1 = metrics.u1;
        quad.v1 = metrics.v1;
        outQuads.Append(quad);

        x += advancePx;
    }
    (void)truncated;
}

String TextLayout::SerializeQuads(const DynamicArray<TextQuad> &quads)
{
    String out;
    const size_t count = quads.GetNElements();
    for (size_t i = 0; i < count; i++)
    {
        const TextQuad &q = quads[i];
        String line = Str<char16_t>::FromFloat(q.x0, 6) + String(u" ") +
                      Str<char16_t>::FromFloat(q.y0, 6) + String(u" ") +
                      Str<char16_t>::FromFloat(q.x1, 6) + String(u" ") +
                      Str<char16_t>::FromFloat(q.y1, 6) + String(u" ") +
                      Str<char16_t>::FromFloat(q.u0, 6) + String(u" ") +
                      Str<char16_t>::FromFloat(q.v0, 6) + String(u" ") +
                      Str<char16_t>::FromFloat(q.u1, 6) + String(u" ") +
                      Str<char16_t>::FromFloat(q.v1, 6);
        if (out.Length() > 0)
            out = out + String(u"\n");
        out = out + line;
    }
    return out;
}
