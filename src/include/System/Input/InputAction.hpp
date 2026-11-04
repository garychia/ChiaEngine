#ifndef INPUT_ACTION_HPP
#define INPUT_ACTION_HPP

#include <cstdint>

// InputAction:標準遊戲輸入動作列舉。
// 由 InputRuntime 將原始按鍵/滑鼠按鈕轉譯為遊戲語義動作。
enum class InputAction : uint32_t
{
    None = 0,

    // 移動
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,

    // 動作
    Jump,
    Action,      // 確認/互動
    Cancel,      // 取消/返回
    Menu,        // 選單

    // 遊戲控制
    Pause,
    Sprint,
    Crouch,
    Interact,

    Count        //  sentinel:動作總數
};

#endif // INPUT_ACTION_HPP
