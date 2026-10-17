#ifndef PONG_HUD_HPP
#define PONG_HUD_HPP

 // PongHud — Pong 的 HUD 模組(Sim 層,確定性,無 GPU)。
 //
 // 每 tick 從 PongSystem 讀分數/勝利狀態,把文字/幾何資訊寫進一份
 // 「View 可讀的 pod」DynamicArray<PongHudLine>。View(SceneWindow::Render)
 // 每幀讀這份 pod,再把它投影成 Frame 的 DrawText 命令(文字在 NDC 空間,
 // 與按鈕文字同一套 px→NDC 慣例,見 GUIFrameProjector)。
 //
 // 它是「Sim → View」的最小接縫:HUD 不知道視窗尺寸、不知道字型、不知道
 // GPU。View 端才知道視窗 size,才能把 NDC 文字擺對。這裡只保證「內容
 // 確定」:相同分數/狀態 → 相同文字序列 → 相同渲染。
 //
 // 分數顯示:左右各一行 "0".."5"。
 // 標題:只要沒有勝利者就顯示 "PONG — W/S 或 ↑/↓ 移動,空白發球"。
 // 勝利:半透明大字 "PLAYER 1 WINS!"(或 2),以及 "Press R to restart"。

#include "Data/DynamicArray.hpp"
#include "Data/String.hpp"
#include "System/Module/EngineContext.hpp"
#include "System/Module/IModule.hpp"
#include "PongSystem.hpp"

#include <cstdint>

namespace pong
{

struct PongHudLine
{
    String text;
    float x;      // NDC x(左上錨點,-1..1;View 端再依字寬微調)
    float y;      // NDC y(左上錨點,-1..1,GL y-up)
    float size;   // 字形像素高度(6..40)
    float r, g, b, a; // 顏色
};

class PongHud : public IModule
{
  public:
    PongHud() : pSystem(nullptr), lines()
    {
    }

    // View 每幀呼叫:回傳目前 HUD 內容(同 tick 的確定性內容)。
    const DynamicArray<PongHudLine> &GetLines() const
    {
        return lines;
    }

    void OnAttach(EngineContext &context) override
    {
        pSystem = context.ResolveService<PongSystem>();
    }

    void OnDetach(EngineContext &context) override
    {
        (void)context;
    }

    // 每 tick 重建決定性 HUD 內容(字串無狀態,永遠新鮮)。
    void FixedUpdate(const FrameClock &clock) override
    {
        (void)clock;
        Rebuild();
    }

    void Update(const FrameClock &clock) override
    {
        (void)clock;
    }

  private:
    PongSystem *pSystem;
    DynamicArray<PongHudLine> lines;

    static String UIntToString(uint32_t value)
    {
        return Str<char16_t>::FromInt(value);
    }

    static String WCharString(const char16_t *pText)
    {
        return String(pText);
    }

    void Rebuild()
    {
        lines.RemoveAll();
        if (!pSystem)
            return;

        const bool hasWinner = pSystem->HasWinner();
        const uint32_t winner = pSystem->GetWinner();

        // 分數(左上/右上,大字)
        {
            PongHudLine line;
            line.text = UIntToString(pSystem->GetLeftScore());
            line.x = -0.88f;
            line.y = 0.78f;
            line.size = 28.0f;
            line.r = 0.95f;
            line.g = 0.85f;
            line.b = 0.25f;
            line.a = 1.0f;
            lines.Append(line);
        }
        {
            PongHudLine line;
            line.text = UIntToString(pSystem->GetRightScore());
            line.x = 0.82f;
            line.y = 0.78f;
            line.size = 28.0f;
            line.r = 0.95f;
            line.g = 0.85f;
            line.b = 0.25f;
            line.a = 1.0f;
            lines.Append(line);
        }

        if (hasWinner)
        {
            PongHudLine line;
            line.text = (winner == 1) ? WCharString(u"PLAYER 1 WINS!") : WCharString(u"PLAYER 2 WINS!");
            line.x = -0.5f;
            line.y = 0.1f;
            line.size = 40.0f;
            line.r = 1.0f;
            line.g = 0.3f;
            line.b = 0.3f;
            line.a = 0.9f;
            lines.Append(line);

            PongHudLine sub;
            sub.text = WCharString(u"Press R to restart");
            sub.x = -0.32f;
            sub.y = -0.15f;
            sub.size = 16.0f;
            sub.r = 0.8f;
            sub.g = 0.8f;
            sub.b = 0.8f;
            sub.a = 0.8f;
            lines.Append(sub);
        }
        else
        {
            // 中間提示:未發球時顯示控制說明
            PongHudLine hint;
            hint.text = WCharString(u"W/S or Up/Down move  Space launch  R restart");
            hint.x = -0.55f;
            hint.y = 0.55f;
            hint.size = 14.0f;
            hint.r = 0.6f;
            hint.g = 0.7f;
            hint.b = 0.9f;
            hint.a = 0.8f;
            lines.Append(hint);
        }
    }
};

} // namespace pong

#endif // PONG_HUD_HPP