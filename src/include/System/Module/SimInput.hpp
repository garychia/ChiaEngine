#ifndef SIM_INPUT_HPP
#define SIM_INPUT_HPP

#include "Data/Padding.hpp"

#include <cstdint>

// 每個 tick 的輸入快照 — Sim 層與輸入來源之間的合約。
// 語意由 Sim 自行定義(actionBits 每個位元的意義、axis 的方向)。
// View 層把真實鍵盤/滑鼠/搖桿轉成 SimInput;replay 時直接把錄製的 SimInput 餵回。
// 保持 trivially copyable:SimRecorder 依賴位元組層級複製。
struct SimInput
{
    uint32_t actionBits = 0;  // action 位元遮罩(例:bit0 = 移動)
    float axisX = 0.0f;       // 軸向輸入(例:水平移動 / 滑鼠 delta X)
    float axisY = 0.0f;       // 軸向輸入(例:垂直移動 / 滑鼠 delta Y)
};

// 兩條契約一起把關:trivially copyable(SimRecorder 的位元組複製)與無隱含 padding
// (避免錄製的輸入位元組受 padding 影響)。見 #142。
static_assert(std::is_trivially_copyable_v<SimInput>, "SimInput must stay trivially copyable.");
CHIA_PADDING_FREE(SimInput, &SimInput::actionBits, &SimInput::axisX, &SimInput::axisY);

#endif // SIM_INPUT_HPP
