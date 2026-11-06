#ifndef ASTEROID_HUD_HPP
#define ASTEROID_HUD_HPP

 // AsteroidHud — Asteroids 的 HUD 模組(Sim 層,確定性,無 GPU)。
 //
 // 每 tick 從 AsteroidSystem 讀得分/生命/波次/小行星剩餘數,生成
 // DynamicArray<AsteroidHudLine> 供 View 端投影成 DrawText。

#include "Data/DynamicArray.hpp"
#include "Data/String.hpp"
#include "System/Module/EngineContext.hpp"
#include "System/Module/IModule.hpp"
#include "AsteroidSystem.hpp"

#include <cstdint>

namespace asteroid
{

struct AsteroidHudLine
{
    String text;
    float x;
    float y;
    float size;
    float r, g, b, a;
};

class AsteroidHud : public IModule
{
  public:
    AsteroidHud() : pSystem(nullptr), lines()
    {
    }

    const DynamicArray<AsteroidHudLine> &GetLines() const { return lines; }

    void OnAttach(EngineContext &context) override
    {
        pSystem = context.ResolveService<AsteroidSystem>();
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
    AsteroidSystem *pSystem;
    DynamicArray<AsteroidHudLine> lines;

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

        // 标题
        if (!gameOver)
        {
            AsteroidHudLine line;
            line.text = WCharString(u"ASTEROIDS — ←/→ 旋转,↑ 加速,Space 射击");
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
            AsteroidHudLine line;
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
            AsteroidHudLine line;
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

        // 波次
        {
            AsteroidHudLine line;
            line.text = WCharString(u"波次: ") + UIntToString(pSystem->GetWave());
            line.x = -0.15f;
            line.y = 0.7f;
            line.size = 16.0f;
            line.r = 0.8f;
            line.g = 0.8f;
            line.b = 0.8f;
            line.a = 0.9f;
            lines.Append(line);
        }

        // 小行星剩余
        {
            AsteroidHudLine line;
            line.text = WCharString(u"小行星: ") +
                        UIntToString(pSystem->GetAsteroidsRemaining());
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
            AsteroidHudLine line;
            line.text = WCharString(u"遊戲結束");
            line.x = -0.2f;
            line.y = 0.1f;
            line.size = 40.0f;
            line.r = 1.0f;
            line.g = 0.3f;
            line.b = 0.3f;
            line.a = 0.9f;
            lines.Append(line);

            AsteroidHudLine sub;
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

} // namespace asteroid

#endif // ASTEROID_HUD_HPP
