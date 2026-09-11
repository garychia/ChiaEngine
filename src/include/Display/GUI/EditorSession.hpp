#ifndef EDITOR_SESSION_HPP
#define EDITOR_SESSION_HPP

#include "Display/GUI/Selection.hpp"
#include "Data/DynamicArray.hpp"
#include "Data/String.hpp"

// ADR-0001 D4: window-agnostic editor state.
// Holds the single-truth-source Selection + undo/redo stack.
// Panel is the View; EditorSession is the model — no GLFW/Vulkan deps.

// ===== EditorCommand (simple value-based undo/redo) =====

class EditorCommand
{
  public:
    virtual ~EditorCommand() = default;
    virtual void Apply() = 0;
    virtual void Revert() = 0;
    virtual String Description() const = 0;
};

// ===== UndoStack: owns EditorCommand* via raw pointer =====

class UndoStack
{
  public:
    UndoStack() = default;

    ~UndoStack()
    {
        for (size_t i = 0; i < mCommands.GetNElements(); i++)
            delete mCommands[i];
    }

    // Take ownership of cmd; clears redo history, then APPLIES it.
    // (Command pattern: push = execute. Undo reverts the last, Redo re-applies.)
    void Push(EditorCommand *cmd)
    {
        if (!cmd)
            return;
        ClearRedo();
        mCommands.Append(cmd);
        cmd->Apply();
    }

    bool Undo()
    {
        if (mCommands.GetNElements() == 0)
            return false;
        EditorCommand *cmd = mCommands[mCommands.GetNElements() - 1];
        cmd->Revert();
        mRedo.Append(cmd);
        mCommands.RemoveLast();
        return true;
    }

    bool Redo()
    {
        if (mRedo.GetNElements() == 0)
            return false;
        EditorCommand *cmd = mRedo[mRedo.GetNElements() - 1];
        cmd->Apply();
        mCommands.Append(cmd);
        mRedo.RemoveLast();
        return true;
    }

    size_t GetUndoCount() const { return mCommands.GetNElements(); }
    size_t GetRedoCount() const { return mRedo.GetNElements(); }
    bool CanUndo() const { return mCommands.GetNElements() > 0; }
    bool CanRedo() const { return mRedo.GetNElements() > 0; }

    void Clear()
    {
        for (size_t i = 0; i < mCommands.GetNElements(); i++)
            delete mCommands[i];
        mCommands.RemoveAll();
        ClearRedo();
    }

  private:
    DynamicArray<EditorCommand *> mCommands;
    DynamicArray<EditorCommand *> mRedo;

    void ClearRedo()
    {
        for (size_t i = 0; i < mRedo.GetNElements(); i++)
            delete mRedo[i];
        mRedo.RemoveAll();
    }

    UndoStack(const UndoStack &) = delete;
    UndoStack &operator=(const UndoStack &) = delete;
};

// ===== EditorSession: window-agnostic editor state =====

class EditorSession
{
  public:
    EditorSession();
    ~EditorSession();

    // Selection (ADR-0001 D2: single truth source)
    Selection &GetSelection() { return mSelection; }
    const Selection &GetSelection() const { return mSelection; }
    void SetSelection(uint32_t entityIndex, bool hasSelection = true);
    void ClearSelection();

    // Undo/Redo
    UndoStack &GetUndoStack() { return mUndoStack; }
    const UndoStack &GetUndoStack() const { return mUndoStack; }

    // Reset all state
    void Reset();

  private:
    Selection mSelection;
    UndoStack mUndoStack;

    EditorSession(const EditorSession &) = delete;
    EditorSession &operator=(const EditorSession &) = delete;
};

#endif // EDITOR_SESSION_HPP
