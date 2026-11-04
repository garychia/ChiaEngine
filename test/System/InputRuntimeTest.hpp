#ifndef INPUT_RUNTIME_TEST_HPP
#define INPUT_RUNTIME_TEST_HPP

#include "System/Input/InputAction.hpp"
#include "System/Input/InputRuntime.hpp"
#include "System/Input/KeyCodes.hpp"
#include "Test.hpp"

#include <cstdio>

namespace inputruntime_test
{

// InputRuntimeTest:A3.4 輸入運行時契約。
// 1. 綁定/查詢輸入動作
// 2. 按下/釋放/按住狀態正確
// 3. 多個綁定映射同一動作
class InputRuntimeTest : public Test
{
  public:
    InputRuntimeTest() : Test("InputRuntime")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:綁定/查詢輸入動作 ----
        {
            InputRuntime input;

            // 建立按鍵組合 A
            Array<KeyCode> keysA = {KeyCodeA};
            KeyCombination comboA(keysA);
            input.RegisterBinding(InputAction::Jump, comboA);

            EXPECT_TRUE(input.GetBindingCount() == 1, "綁定 1 個動作", true);

            // 未按下 → 所有狀態都為 false
            input.BeginFrame();
            input.EndFrame();
            EXPECT_TRUE(!input.IsActionPressed(InputAction::Jump),
                        "未按下 → Pressed=false", true);
            EXPECT_TRUE(!input.IsActionReleased(InputAction::Jump),
                        "未按下 → Released=false", true);
            EXPECT_TRUE(!input.IsActionHeld(InputAction::Jump),
                        "未按下 → Held=false", true);
        }

        // ---- AC2:按下/釋放/按住狀態 ----
        {
            InputRuntime input;

            Array<KeyCode> keysA = {KeyCodeA};
            KeyCombination comboA(keysA);
            input.RegisterBinding(InputAction::Jump, comboA);

            // 第 1 幀:BeginFrame → 按下 A → 檢查 Pressed/Held
            input.BeginFrame();
            input.SetKeyDown(static_cast<unsigned int>(KeyCodeA));
            input.EndFrame();
            EXPECT_TRUE(input.IsActionPressed(InputAction::Jump),
                "第 1 幀按下 → Pressed=true", true);
            EXPECT_TRUE(input.IsActionHeld(InputAction::Jump),
                "第 1 幀按下 → Held=true", true);
            EXPECT_TRUE(!input.IsActionReleased(InputAction::Jump),
                "第 1 幀按下 → Released=false", true);

            // 第 2 幀:繼續按住 A
            input.BeginFrame();
            input.EndFrame();
            EXPECT_TRUE(!input.IsActionPressed(InputAction::Jump),
                "第 2 幀繼續按住 → Pressed=false", true);
            EXPECT_TRUE(input.IsActionHeld(InputAction::Jump),
                "第 2 幀繼續按住 → Held=true", true);
            EXPECT_TRUE(!input.IsActionReleased(InputAction::Jump),
                "第 2 幀繼續按住 → Released=false", true);

            // 第 3 幀:釋放 A
            input.BeginFrame();
            input.SetKeyUp(static_cast<unsigned int>(KeyCodeA));
            input.EndFrame();
            EXPECT_TRUE(!input.IsActionPressed(InputAction::Jump),
                "第 3 幀釋放 → Pressed=false", true);
            EXPECT_TRUE(!input.IsActionHeld(InputAction::Jump),
                "第 3 幀釋放 → Held=false", true);
            EXPECT_TRUE(input.IsActionReleased(InputAction::Jump),
                "第 3 幀釋放 → Released=true", true);

            // 第 4 幀:保持釋放
            input.BeginFrame();
            input.EndFrame();
            EXPECT_TRUE(!input.IsActionPressed(InputAction::Jump),
                "第 4 幀保持釋放 → Pressed=false", true);
            EXPECT_TRUE(!input.IsActionReleased(InputAction::Jump),
                "第 4 幀保持釋放 → Released=false", true);
        }

        // ---- AC3:多個綁定映射同一動作 ----
        {
            InputRuntime input;

            Array<KeyCode> keysSpace = {KeyCodeSpace};
            Array<KeyCode> keysW = {KeyCodeW};
            KeyCombination comboSpace(keysSpace);
            KeyCombination comboW(keysW);
            input.RegisterBinding(InputAction::Jump, comboSpace);
            input.RegisterBinding(InputAction::Jump, comboW);

            EXPECT_TRUE(input.GetBindingCount() == 2, "綁定 2 個動作到同一 InputAction", true);

            // 按下 Space → Jump 應被觸發
            input.BeginFrame();
            input.SetKeyDown(static_cast<unsigned int>(KeyCodeSpace));
            input.EndFrame();
            EXPECT_TRUE(input.IsActionPressed(InputAction::Jump),
                "按下 Space → Jump Pressed=true", true);

            // 釋放 Space,按下 W
            input.BeginFrame();
            input.SetKeyUp(static_cast<unsigned int>(KeyCodeSpace));
            input.SetKeyDown(static_cast<unsigned int>(KeyCodeW));
            input.EndFrame();
            EXPECT_TRUE(input.IsActionPressed(InputAction::Jump),
                "按下 W → Jump Pressed=true", true);
        }

        // ---- AC4:UnregisterBinding 移除綁定 ----
        {
            InputRuntime input;

            Array<KeyCode> keysA = {KeyCodeA};
            KeyCombination comboA(keysA);
            input.RegisterBinding(InputAction::Jump, comboA);
            input.RegisterBinding(InputAction::Action, comboA);

            EXPECT_TRUE(input.GetBindingCount() == 2, "註冊 2 個綁定", true);

            input.UnregisterBinding(InputAction::Jump, comboA);
            EXPECT_TRUE(input.GetBindingCount() == 1, "Unregister 後剩 1 個綁定", true);
            EXPECT_TRUE(!input.IsActionPressed(InputAction::Jump),
                        "Unregister Jump 後 → Jump 不再綁定", true);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }

  private:
};

} // namespace inputruntime_test

#endif // INPUT_RUNTIME_TEST_HPP
