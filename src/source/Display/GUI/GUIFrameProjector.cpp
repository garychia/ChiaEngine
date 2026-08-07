#include "Display/GUI/GUIFrameProjector.hpp"

#include "Display/GUI/Button.hpp"
#include "Display/Text/TextLayout.hpp"

void GUIFrameProjector::ProjectLabels(GUILayout &layout, const GlyphAtlas &atlas, uint64_t fontId, Frame &frame)
{
    const DynamicArray<SharedPtr<GUILayer>> &layers = layout.GetLayers();
    for (size_t li = 0; li < layers.GetNElements(); li++)
    {
        const DynamicArray<SharedPtr<IGUI>> &components = layers[li]->GetComponents();
        for (size_t ci = 0; ci < components.GetNElements(); ci++)
        {
            const Button *pButton = dynamic_cast<const Button *>(components[ci].operator->());
            if (pButton == nullptr)
                continue;
            const String &label = pButton->GetLabel();
            if (label.Length() == 0)
                continue;

            const Point2D windowSize = pButton->GetWindowSize();
            const float size = pButton->GetFontSize();
            if (windowSize.x <= 0.0f || windowSize.y <= 0.0f || size <= 0.0f)
                continue;

            // 以同一個佈局演算文字像素尺寸 → 把文字塊置中在按鈕上。
            // 與執行器的 TextLayout 使用相同 size(命令的 textSize)→ 兩邊一致(R4)。
            DynamicArray<TextQuad> quads;
            TextLayout::Layout(atlas, label.CStr(), size, 0.0f, quads);
            float textWidth = 0.0f;
            float textHeight = 0.0f;
            for (size_t qi = 0; qi < quads.GetNElements(); qi++)
            {
                if (quads[qi].x1 > textWidth)
                    textWidth = quads[qi].x1;
                if (quads[qi].y1 > textHeight)
                    textHeight = quads[qi].y1;
            }

            // 按鈕中心(px)→ 文字錨點(px,左上)置中
            const Point2D topLeft = pButton->GetTopLeftPosition();
            const float centerPxX = topLeft.x + pButton->GetWidth() * 0.5f;
            const float centerPxY = topLeft.y + pButton->GetHeight() * 0.5f;
            const float anchorPxX = centerPxX - textWidth * 0.5f;
            const float anchorPxY = centerPxY - textHeight * 0.5f;

            // px → NDC(與 IGUI::SetTopLeftPosition 同一慣例)
            const float ndcX = anchorPxX / windowSize.x * 2.0f - 1.0f;
            const float ndcY = 1.0f - anchorPxY / windowSize.y * 2.0f;
            const float scaleX = 2.0f / windowSize.x;
            const float scaleY = -2.0f / windowSize.y;
            // z:比按鈕稍微靠近相機,避免 depth 平排時被按鈕幾何吃掉
            const float z = components[ci]->GetPosition().z + 0.01f;

            frame.PushTransform(Point3D(ndcX, ndcY, z), Point3D(), Point3D(scaleX, scaleY, 1.0f));
            frame.DrawText(fontId, label, size, pButton->GetTextColor());
            frame.PopTransform();
        }
    }
}
