#ifndef GLYPH_ATLAS_HPP
#define GLYPH_ATLAS_HPP

#include "Data/DynamicArray.hpp"
#include "Types/Types.hpp"

#include <cstdint>

// 內建點陣字型圖集(bitmap font atlas)— 純資料,無 GPU 依賴。
// 來源:public-domain 8x8 pixel font(font8x8_basic,CC0),以 bit 表格內嵌,
// 建構時光柵成 RGBA(白字透明底),內容位元組確定 → DefaultFontId() 內容定址。
// 圖集布局:16 欄 × 8 列,每格 8x8 px、格間 1 px padding,ASCII 0x20..0x7E。
struct GlyphMetrics
{
    float u0; // atlas UV(0..1),左上
    float v0;
    float u1; // 右下
    float v1;
    float advance; // 前進量,字型單位(= size 縮放基準,此字型為 8)

    GlyphMetrics(float u0 = 0, float v0 = 0, float u1 = 0, float v1 = 0, float advance = 8)
        : u0(u0), v0(v0), u1(u1), v1(v1), advance(advance)
    {
    }
};

class GlyphAtlas
{
  private:
    uint32_t atlasWidth;
    uint32_t atlasHeight;
    DynamicArray<unsigned char> rgba;

    static constexpr uint32_t CellSize = 8;          // 每格 8x8 px
    static constexpr uint32_t AtlasColumns = 16;     // 16 欄
    static constexpr uint32_t AtlasRows = 8;         // 8 列(128 格,使用 0x20..0x7E 共 95)
    static constexpr uint32_t CellPitch = CellSize + 1; // 含 padding

    void Rasterize();
    uint32_t GlyphIndex(char16_t ch) const; // 非 0x20..0x7E → fallback 空白(index 0)

  public:
    GlyphAtlas();
    GlyphAtlas(const GlyphAtlas &other);

    GlyphAtlas &operator=(const GlyphAtlas &other);

    // 共享的預設字型實例(靜態單例,內容確定)與其內容定址 id。
    static const GlyphAtlas &GetDefault();
    static uint64_t DefaultFontId();

    uint32_t GetAtlasWidth() const;
    uint32_t GetAtlasHeight() const;
    const unsigned char *GetRGBA() const;

    // 回傳該字元的 UV 與 advance;非 ASCII(或不可列印)→ fallback 空白。
    GlyphMetrics GetMetrics(char16_t ch) const;
};

#endif // GLYPH_ATLAS_HPP
