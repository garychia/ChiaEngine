#ifndef PLATFORMER_HUD_HPP
#define PLATFORMER_HUD_HPP

 // PlatformerHud — 平台跳躍的 HUD 模組(Sim 層,確定性,無 GPU)。
 //
 // 每 tick 從 PlatformerSystem 讀得分/生命/是否在地面,生成
 // DynamicArray<PlatformerHudLine> 供 View 端投影成 DrawText。

#include "Data/DynamicArray.hpp"
#include "Data/String.hpp"
#include "System/Module/EngineContext.hpp"
#include "System/Module/IModule.hpp"
#include "PlatformerSystem.hpp"

#include <cstdint>

namespace platformer
{

struct PlatformerHudLine
{
    String text;
    float x;
    float y;
    float size;
    float r, g, b, a;
};

class PlatformerHud : public IModule
{
  public:
    PlatformerHud() : pSystem(nullptr), lines()
    {
    }

    const DynamicArray<PlatformerHudLine> &GetLines() const { return lines; }

    void OnAttach(EngineContext &context) override
    {
        pSystem = context.ResolveService<PlatformerSystem>();
    }

    void OnDetach(EngineContext &context) override
    {
        (void)context;
    }

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
    PlatformerSystem *pSystem;
    DynamicArray<PlatformerHudLine> lines;

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

        const bool gameOver = pSystem->IsGameOver();
        const bool onGround = pSystem->IsOnGround();

        // 标题
        if (!gameOver)
        {
            PlatformerHudLine line;
            line.text = WCharString(u"PLATFORMER — ←/→ 移動,Space 跳躍");
            line.x = -0.65f;
            line.y = 0.85f;
            line.size = 14.0f;
            line.r = 0.6f;
            line.g = 0.7f;
            line.b = 0.9f;
            line.a = 0.8f;
            lines.Append(line);
        }

        // 得分
        {
            PlatformerHudLine line;
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

        // 生命
        {
            PlatformerHudLine line;
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

        // 地面狀態
        {
            PlatformerHudLine line;
            line.text = WCharString(u"地面: ") +
                        WCharString(onGround ? u"是" : u"否");
            line.x = -0.45f;
            line.y = 0.7f;
            line.size = 16.0f;
            line.r = 0.8f;
            line.g = 0.8f;
            line.b = 0.8f;
            line.a = 0.9f;
            lines.Append(line);
        }

        if (gameOver)
        {
            PlatformerHudLine line;
            line.text = WCharString(u"遊戲結束");
            line.x = -0.2f;
            line.y = 0.1f;
            line.size = 40.0f;
            line.r = 1.0f;
            line.g = 0.3f;
            line.b = 0.3f;
            line.a = 0.9f;
            lines.Append(line);

            PlatformerHudLine sub;
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
};

} // namespace platformer

#endif // PLATFORMER_HUD_HPP
