#ifndef GUI_FRAME_PROJECTOR_HPP
#define GUI_FRAME_PROJECTOR_HPP

#include "Data/String.hpp"
#include "Display/Frame.hpp"
#include "Display/GUI/GUILayout.hpp"
#include "Display/Text/GlyphAtlas.hpp"

// View 層投影器:把 GUI 佈局「投影」成 Frame 命令。
// 按鈕幾何仍走 DrawGUILayout(相容),label 在此以 DrawText 命令進入同一份 Frame —
// 呼應架構 §4.3「GUI 變成 GUI 模組,也輸出 Frame 命令」。
// 對每顆有 label 的 Button:
//   PushTransform(按鈕中心 NDC + px→NDC scale) → DrawText → PopTransform
// px→NDC 慣例與 IGUI::SetTopLeftPosition 一致(原點左上、y 向下);scale.y 為負,
// 讓文字 quad 的「向下 y」在螢幕空間也向下(集中一處,執行器不再各自換算,見 RFC R4)。
class GUIFrameProjector
{
  public:
    // layout:要投影的佈局;atlas/fontId:label 使用的字型與其內容定址 id;
    // frame:輸出目標(DrawGUILayout 之後呼叫,label 附加在後)。
    static void ProjectLabels(GUILayout &layout, const GlyphAtlas &atlas, uint64_t fontId, Frame &frame);
};

#endif // GUI_FRAME_PROJECTOR_HPP
