#ifndef EDITOR_SESSION_HPP
#define EDITOR_SESSION_HPP

#include "Data/DynamicArray.hpp"
#include "Data/String.hpp"
#include "Display/GUI/PanelPane.hpp"
#include "Display/GUI/Selection.hpp"
#include "Scene/SceneSystem.hpp"
#include "System/Module/CameraController.hpp"
#include "System/Module/SimRecorder.hpp"

// ADR-0001 D4:window-agnostic editor 狀態與組合根。
// 擁有 pointer set(SceneSystem* / CameraController* / SimRecorder*)+
// 單一真相來源 Selection + undo/redo stack + PanelPane registry。
// Panel 是 View;EditorSession 是 model — 不包含任何 GLFW/Window/型別,
// 因此整顆編輯器可無頭測試(不需 GPU/視窗)。
// (Display→Sim 方向;Sim 頭檔仍不 include Display — 層規則不變。)

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
    // ADR-0001 D4:注入 pointer set(不擁有 — Panel/App 長時間持有)。
    // 全部可為 nullptr(測試 / 未接線無妨)。
    EditorSession(SceneSystem *pSceneSystem = nullptr, CameraController *pCameraController = nullptr,
                  SimRecorder *pSimRecorder = nullptr);
    ~EditorSession();

    // ---- D4 pointer set ----
    SceneSystem *GetSceneSystem() const { return mpSceneSystem; }
    CameraController *GetCameraController() const { return mpCameraController; }
    SimRecorder *GetSimRecorder() const { return mpSimRecorder; }

    // ---- Selection (ADR-0001 D2:single truth source) ----
    Selection &GetSelection() { return mSelection; }
    const Selection &GetSelection() const { return mSelection; }
    // 編輯器選取同步:只更新 session 的 Selection(View 端再刷新 inspector/highlight)。
    void Select(uint32_t entityIndex);
    void ClearSelection();

    // ---- Undo/Redo ----
    UndoStack &GetUndoStack() { return mUndoStack; }
    const UndoStack &GetUndoStack() const { return mUndoStack; }
    // Editor 快捷鍵(Ctrl+Z undo / Ctrl+Y redo)。回傳 true = 已消費
    // (與 SimRecorder 的 F5/F6 gameplay replay 完全分離)。無 Window 依賴,可無頭測。
    bool HandleEditorShortcut(bool hasCtrl, bool hasZ, bool hasY);

    // ---- D3/D4 PanelPane registry ----
    // 註冊 named pane(按 title 為 key)。重複 title 回傳既有 pane,不新增。
    PanelPane *RegisterPane(SharedPtr<PanelPane> pPane);
    PanelPane *GetPane(const String &title);
    const DynamicArray<SharedPtr<PanelPane>> &GetPanes() const { return mPanes; }
    void SetPaneCollapsed(const String &title, bool collapsed); // 收合 → dock 高度歸零

    // Reset all state
    void Reset();

  private:
    SceneSystem *mpSceneSystem;
    CameraController *mpCameraController;
    SimRecorder *mpSimRecorder;
    Selection mSelection;
    UndoStack mUndoStack;
    DynamicArray<SharedPtr<PanelPane>> mPanes;

    EditorSession(const EditorSession &) = delete;
    EditorSession &operator=(const EditorSession &) = delete;
};

#endif // EDITOR_SESSION_HPP