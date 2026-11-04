#ifndef INPUT_RUNTIME_HPP
#define INPUT_RUNTIME_HPP

#include "Data/DynamicArray.hpp"
#include "System/Input/InputAction.hpp"
#include "System/Input/KeyCombination.hpp"

#include <cstdint>

// InputBinding:單個輸入綁定(KeyCombination → InputAction)。
struct InputBinding
{
    InputAction action = InputAction::None;
    KeyCombination keys;

    InputBinding() = default;
    InputBinding(InputAction a, const KeyCombination &k) : action(a), keys(k) {}
};

// InputRuntime:輸入運行時系統，管理輸入綁定與幀輸入狀態查詢。
// 流程:
//  1. RegisterBinding():註冊 KeyCombination → InputAction 綁定
//  2. SetKeyDown/SetKeyUp():由平台輸入系統(KeyboardHandler 等)呼叫,更新按鍵狀態
//  3. BeginFrame/EndFrame():幀邊界,計算 pressed/released/held 狀態
//  4. IsActionPressed/IsActionReleased/IsActionHeld():查詢動作狀態
class InputRuntime
{
  public:
    InputRuntime();
    ~InputRuntime();

    InputRuntime(const InputRuntime &) = delete;
    InputRuntime &operator=(const InputRuntime &) = delete;

    // 註冊輸入綁定。同一動作可綁定多個按鍵組合。
    void RegisterBinding(InputAction action, const KeyCombination &keys);

    // 移除輸入綁定。
    void UnregisterBinding(InputAction action, const KeyCombination &keys);

    // 由平台輸入系統呼叫:標記按鍵按下/釋放。
    void SetKeyDown(unsigned int keyCode);
    void SetKeyUp(unsigned int keyCode);

    // 幀開始:將目前 held 狀態轉為 previous,準備新幀。
    void BeginFrame();

    // 幀結束:計算 pressed/released。
    void EndFrame();

    // 動作狀態查詢
    bool IsActionPressed(InputAction action) const;   // 本剛按下(previous=false,current=true)
    bool IsActionReleased(InputAction action) const;  // 本剛釋放(previous=true,current=false)
    bool IsActionHeld(InputAction action) const;      // 按住中(current=true)

    // 取得綁定數量。
    size_t GetBindingCount() const { return mBindings.GetNElements(); }

  private:
    // 檢查按鍵組合是否全部按下。
    bool IsCombinationDown(const KeyCombination &keys) const;

    DynamicArray<InputBinding> mBindings;

    // 按鍵狀態:256 個鍵位(current/previous)
    bool mKeyCurrent[256];
    bool mKeyPrevious[256];
};

#endif // INPUT_RUNTIME_HPP
