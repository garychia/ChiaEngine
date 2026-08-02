#ifndef FRAMECLOCK_HPP
#define FRAMECLOCK_HPP

#include <cstdint>

// 確定性時鐘:由 Engine 驅動,不讀牆鐘。
// Sim 層只准使用這裡的時間 — 這是「同樣輸入 → 同樣結果」可重播的基礎。
struct FrameClock
{
    uint64_t frameIndex = 0;                // 渲染幀編號(每次 Update +1)
    uint64_t tickIndex = 0;                 // 模擬 tick 編號(每次 FixedUpdate +1)
    double frameDeltaSeconds = 0.0;         // 本幀間隔(Update 用)
    double fixedDeltaSeconds = 1.0 / 60.0;  // 固定步進(FixedUpdate 用)
};

#endif // FRAMECLOCK_HPP
