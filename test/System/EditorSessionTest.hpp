#ifndef EDITOR_SESSION_TEST_HPP
#define EDITOR_SESSION_TEST_HPP

#include "Display/GUI/EditorSession.hpp"
#include "Test.hpp"

// #72 ADR-0001 D4:EditorSession 是 window-agnostic 編輯器模型
// (Selection + undo/redo),完全不依賴 GLFW/Vulkan → 可 headless 測試。
namespace editorsessiontest
{

// Test-local command: 對一個 float 做 set(記錄舊值)。
class FloatSetCommand : public EditorCommand
{
  public:
    FloatSetCommand(float *pTarget, float newValue, const String &label)
        : mpTarget(pTarget), mOldValue(*pTarget), mNewValue(newValue), mLabel(label)
    {
    }

    void Apply() override { *mpTarget = mNewValue; }
    void Revert() override { *mpTarget = mOldValue; }
    String Description() const override { return mLabel; }

  private:
    float *mpTarget;
    float mOldValue;
    float mNewValue;
    String mLabel;
};

class EditorSessionTest : public Test
{
  public:
    EditorSessionTest() : Test("EditorSession")
    {
    }

    bool Run() noexcept override
    {
        // ---- AC1:Selection 單一真相來源 ----
        {
            EditorSession session;
            EXPECT_TRUE(!session.GetSelection().hasSelection, "初始無選取.", true);
            session.SetSelection(7, true);
            EXPECT_TRUE(session.GetSelection().entityIndex == 7, "SetSelection 更新 entityIndex.", true);
            EXPECT_TRUE(session.GetSelection().hasSelection, "SetSelection 設 hasSelection=true.", true);
        }

        // ---- AC2:Undo/Redo 堆疊 ----
        {
            EditorSession session;
            UndoStack &stack = session.GetUndoStack();

            float value = 0.f;
            stack.Push(new FloatSetCommand(&value, 1.f, String(u"set 1")));
            stack.Push(new FloatSetCommand(&value, 2.f, String(u"set 2")));
            EXPECT_TRUE(stack.GetUndoCount() == 2, "push 兩個 command 後 undoCount==2.", true);

            EXPECT_TRUE(stack.Undo(), "undo 可執行.", true);
            EXPECT_TRUE(value == 1.f, "undo 後 value 回到 1.", true);
            EXPECT_TRUE(stack.GetRedoCount() == 1, "undo 後 redoCount==1.", true);

            EXPECT_TRUE(stack.Redo(), "redo 可執行.", true);
            EXPECT_TRUE(value == 2.f, "redo 後 value 回到 2.", true);
            EXPECT_TRUE(stack.GetUndoCount() == 2 && stack.GetRedoCount() == 0,
                        "redo 後 undo/redo count 正確.", true);
        }

        // ---- AC3:push 新的 command 清空 redo ----
        {
            EditorSession session;
            UndoStack &stack = session.GetUndoStack();
            float value = 0.f;
            stack.Push(new FloatSetCommand(&value, 1.f, String(u"a")));
            stack.Undo();
            EXPECT_TRUE(stack.GetRedoCount() == 1, "undo 後 redoCount==1.", true);
            stack.Push(new FloatSetCommand(&value, 5.f, String(u"b")));
            EXPECT_TRUE(stack.GetRedoCount() == 0, "push 新 command 清空 redo.", true);
        }

        // ---- AC4:Reset 清空 selection 與 undo stack ----
        {
            EditorSession session;
            session.SetSelection(3, true);
            float value = 0.f;
            session.GetUndoStack().Push(new FloatSetCommand(&value, 1.f, String(u"x")));
            session.Reset();
            EXPECT_TRUE(!session.GetSelection().hasSelection, "Reset 後無選取.", true);
            EXPECT_TRUE(session.GetUndoStack().GetUndoCount() == 0, "Reset 後 undo stack 清空.", true);
        }

        SUCCESS_MESSAGE("EditorSession: selection / undo-redo 全部通過.");
        return true;
    }
};

} // namespace editorsessiontest

#endif // EDITOR_SESSION_TEST_HPP