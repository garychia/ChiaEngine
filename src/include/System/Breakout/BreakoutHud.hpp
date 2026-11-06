#ifndef BREAKOUT_HUD_HPP
#define BREAKOUT_HUD_HPP

 // BreakoutHud — Breakout 的 HUD 模組(Sim 層,確定性,無 GPU)。
 //
 // 每 tick 從 BreakoutSystem 讀分數/生命/磚塊剩餘數,把文字資訊寫進一份
 // 「View 可讀的 pod」DynamicArray<BreakoutHudLine>。View(SceneWindow)每幀讀
 // 這份 pod,再把它投影成 Frame 的 DrawText 命令。
 //
 // 它是「Sim → View」的最小接縫:HUD 不知道視窗尺寸、不知道字型、不知道
 // GPU。View 端才知道視窗 size,才能把 NDC 文字擺對。這裡只保證「內容
 // 確定」:相同分數/狀態 → 相同文字序列 → 相同渲染。

#include "Data/DynamicArray.hpp"
#include "Data/String.hpp"
#include "System/Module/EngineContext.hpp"
#include "System/Module/IModule.hpp"
#include "BreakoutSystem.hpp"

#include <cstdint>

namespace breakout
{

struct BreakoutHudLine
{
    String text;
    float x;      // NDC x(左上錨點,-1..1;View 端再依字寬微調)
    float y;      // NDC y(左上錨點,-1..1,GL y-up)
    float size;   // 字形像素高度(6..40)
    float r, g, b, a; // 顏色
};

class BreakoutHud : public IModule
{
  public:
    BreakoutHud() : pSystem(nullptr), lines()
    {
    }

    // View 每幀呼叫:回傳目前 HUD 內容(同 tick 的確定性內容)。
    const DynamicArray<BreakoutHudLine> &GetLines() const
    {
        return lines;
    }

    void OnAttach(EngineContext &context) override
    {
        pSystem = context.ResolveService<BreakoutSystem>();
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
    BreakoutSystem *pSystem;
    DynamicArray<BreakoutHudLine> lines;

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

        // 標題
        if (!hasWinner)
        {
            BreakoutHudLine line;
            line.text = WCharString(u"BREAKOUT — ←/→ 移動, Space 發球, R 重設");
            line.x = -0.6f;
            line.y = 0.85f;
            line.size = 14.0f;
            line.r = 0.6f;
            line.g = 0.7f;
            line.b = 0.9f;
            line.a = 0.8f;
            lines.Append(line);
        }

        // 分數(左上)
        {
            BreakoutHudLine line;
            line.text = WCharString(u"分數: ") + UIntToString(pSystem->GetScore());
            line.x = -0.9f;
            line.y = 0.7f;
            line.size = 20.0f;
            line.r = 0.95f;
            line.g = 0.85f;
            line.b = 0.25f;
            line.a = 1.0f;
            lines.Append(line);
        }

        // 生命(右上)
        {
            BreakoutHudLine line;
            line.text = WCharString(u"生命: ") + UIntToString(pSystem->GetLives());
            line.x = 0.7f;
            line.y = 0.7f;
            line.size = 20.0f;
            line.r = 0.95f;
            line.g = 0.85f;
            line.b = 0.25f;
            line.a = 1.0f;
            lines.Append(line);
        }

        // 磚塊剩餘(中上)
        {
            BreakoutHudLine line;
            line.text = WCharString(u"磚塊: ") + UIntToString(pSystem->GetBricksRemaining());
            line.x = -0.15f;
            line.y = 0.7f;
            line.size = 16.0f;
            line.r = 0.8f;
            line.g = 0.8f;
            line.b = 0.8f;
            line.a = 0.9f;
            lines.Append(line);
        }

        if (hasWinner)
        {
            if (winner == 1)
            {
                BreakoutHudLine line;
                line.text = WCharString(u"你贏了! 所有磚塊已消滅");
                line.x = -0.45f;
                line.y = 0.1f;
                line.size = 40.0f;
                line.r = 0.3f;
                line.g = 1.0f;
                line.b = 0.3f;
                line.a = 0.9f;
                lines.Append(line);

                BreakoutHudLine sub;
                sub.text = WCharString(u"按 R 重新開始");
                sub.x = -0.25f;
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
                BreakoutHudLine line;
                line.text = WCharString(u"遊戲結束  生命歸零");
                line.x = -0.45f;
                line.y = 0.1f;
                line.size = 40.0f;
                line.r = 1.0f;
                line.g = 0.3f;
                line.b = 0.3f;
                line.a = 0.9f;
                lines.Append(line);

                BreakoutHudLine sub;
                sub.text = WCharString(u"按 R 重新開始");
                sub.x = -0.25f;
                sub.y = -0.15f;
                sub.size = 16.0f;
                sub.r = 0.8f;
                sub.g = 0.8f;
                sub.b = 0.8f;
                sub.a = 0.8f;
                lines.Append(sub);
            }
        }
        else if (livesRemaining() > 0)
        {
            // 中間提示:未發球時顯示控制說明
            BreakoutHudLine hint;
            hint.text = WCharString(u"按 Space 發球");
            hint.x = -0.15f;
            hint.y = 0.0f;
            hint.size = 14.0f;
            hint.r = 0.6f;
            hint.g = 0.7f;
            hint.b = 0.9f;
            hint.a = 0.8f;
            lines.Append(hint);
        }
    }

    uint32_t livesRemaining() const
    {
        return pSystem ? pSystem->GetLives() : 0;
    }
};

} // namespace breakout

#endif // BREAKOUT_HUD_HPP
