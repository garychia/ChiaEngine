#ifndef EDITOR_SESSION_HPP
#define EDITOR_SESSION_HPP

#include "Data/DynamicArray.hpp"
#include "Data/String.hpp"
#include "Data/HashTable.hpp"
#include "Data/Hash.hpp"
#include "Display/GUI/PanelPane.hpp"
#include "Display/GUI/Selection.hpp"
#include "Display/Asset/TextureAssetView.hpp"
#include "Display/IRendererAssetRegistrar.hpp"
#include "Scene/SceneSystem.hpp"
#include "System/Asset/AssetManager.hpp"
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
                  SimRecorder *pSimRecorder = nullptr, AssetManager *pAssetManager = nullptr,
                  IRendererAssetRegistrar *pRegistrar = nullptr);
    ~EditorSession();

    // ---- D4 pointer set ----
    SceneSystem *GetSceneSystem() const { return mpSceneSystem; }
    CameraController *GetCameraController() const { return mpCameraController; }
    SimRecorder *GetSimRecorder() const { return mpSimRecorder; }
    AssetManager *GetAssetManager() const { return mpAssetManager; }
    IRendererAssetRegistrar *GetAssetRegistrar() const { return mpRegistrar; }

    // 綁定 AssetManager:TextureAssetView 訂閱其 LoadedEvent,EditorSession 也自行訂閱
    // 以記錄 path → contentHash 映射(供 GetMaterialId 查詢)。
    void SetAssetManager(AssetManager *pAssetManager);
    // 綁定 renderer 資產註冊器:TextureAssetView 的解碼完成 hook 會把 RGBA pixels
    // 透過此 registrar 呼叫 RegisterMaterial(contentHash, {pRawRGBA, w, h}) 上傳 GPU。
    void SetAssetRegistrar(IRendererAssetRegistrar *pRegistrar);

    // 發起貼圖非同步載入(依 key 去重,不重複讀檔)。回傳 pathHash 作為追蹤 ID;
    // 實際 materialId(= contentHash)需在 DispatchAssetEvents 後透過 GetMaterialId 查詢。
    uint64_t LoadTexture(const String &path);

    // 主線程:派發 AssetManager 完成事件 → TextureAssetView 解碼 → hook → RegisterMaterial。
    // 需在每個 Update 幀或載入後呼叫。
    void DispatchAssetEvents();

    // 查詢某路徑貼圖已註冊的 materialId(= contentHash); 未完成回 0。
    uint64_t GetMaterialId(const String &path) const;

    // 查詢某路徑貼圖是否已載入完成(解碼並註冊成功)。
    bool IsTextureLoaded(const String &path) const;

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
    AssetManager *mpAssetManager;
    IRendererAssetRegistrar *mpRegistrar;
    TextureAssetView mTextureView;      // Value:訂閱 AssetManager,解碼 → 轉交 hook
    HashTable<uint64_t, uint64_t> mTextureToContentHash;  // pathHash → contentHash(materialId)
    Selection mSelection;
    UndoStack mUndoStack;
    DynamicArray<SharedPtr<PanelPane>> mPanes;

    // AssetManager::LoadedEvent 回編:記錄 path → contentHash 映射。
    void OnAssetLoaded(SharedPtr<Asset> asset);

    // TextureAssetView hook 靜態進入點:rgba 解碼後經註冊器上傳 GPU。
    static void OnTextureReadyStatic(const unsigned char *rgbaPixels, int width, int height,
                                     size_t contentHash, void *pUserData);

    // 簡易 FNV-1a 字串雜湊(僅用於 EditorSession 內部 path→contentHash 索引)。
    static uint64_t HashPath(const String &path);

    EditorSession(const EditorSession &) = delete;
    EditorSession &operator=(const EditorSession &) = delete;
};

#endif // EDITOR_SESSION_HPP