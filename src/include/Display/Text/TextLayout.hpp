#ifndef TEXT_LAYOUT_HPP
#define TEXT_LAYOUT_HPP

#include "Data/DynamicArray.hpp"
#include "Data/String.hpp"
#include "Display/Text/GlyphAtlas.hpp"

#include <cstdint>

// 文字 → quad 序列的純佈局模組(無 GPU、無 headless 依賴)。
// 這是「Frame 的 DrawText 命令」與「執行器著色」之間的確定性橋樑:
//   佈局在此算 → quad/UV 序列可序列化比對(驗收 1),執行器照樣板上傳。
// 座標慣例:local 像素座標,原點左上、y 向下;字形 box = size × size。
struct TextQuad
{
    float x0;
    float y0;
    float x1;
    float y1;
    float u0;
    float v0;
    float u1;
    float v1;

    TextQuad(float x0 = 0, float y0 = 0, float x1 = 0, float y1 = 0,
             float u0 = 0, float v0 = 0, float u1 = 0, float v1 = 0)
        : x0(x0), y0(y0), x1(x1), y1(y1), u0(u0), v0(v0), u1(u1), v1(v1)
    {
    }
};

class TextLayout
{
  public:
    // 把文字佈局成 quad 序列。
    //  - size:字形像素高度(= glyph box 邊長);lineHeight = size * 1.2。
    //  - maxWidthPx ≤ 0 = 不截斷;> 0 時每行超寬 → 截斷並以 "..." 收尾(為省略號預留空間)。
    //  - 遇 '\n' 換行;非 ASCII → 空白 fallback(不崩潰)。
    //  - text 為 null-terminated char16_t 字串(與 String::CStr() 相容)。
    static void Layout(const GlyphAtlas &atlas, const char16_t *text, float size,
                       float maxWidthPx, DynamicArray<TextQuad> &outQuads);

    // 確定字形序列化:每 quad 一行 "x0 y0 x1 y1 u0 v0 u1 v1"(固定 6 位小數),
    // 相同輸入 → 相同字串;測試比對用。
    static String SerializeQuads(const DynamicArray<TextQuad> &quads);
};

#endif // TEXT_LAYOUT_HPP
