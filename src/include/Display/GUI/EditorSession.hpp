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
#include "Geometry/2D/Point2D.hpp"
#include "Scene/MaterialComponent.hpp"
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

    // ---- 已載入貼圖列表(供 Inspector 材質選擇) ----
    // 已載入貼圖數量(依 LoadTexture 成功順序)。
    size_t GetTextureCount() const;
    // 第 index 張已載入貼圖的路徑(調用者需保證 index < GetTextureCount())。
    const String &GetTexturePathAt(size_t index) const;
    // 將指定路徑貼圖的 materialId 綁定到目前選取的場景節點(= SetMaterial)。
    // 路徑未載入完成或未選取節點則 no-op。
    void BindTextureToSelectedEntity(const String &path);
    // 查詢目前選取場景節點的 materialId(未選取/未綁定回 0)。
    uint64_t GetSelectedEntityMaterialId() const;

    // ---- Scene Edit Commands (Undo/Redo, ADR-0001 D5) ----
    // 將場景編輯操作以 EditorCommand 形式 push 到 UndoStack,支援 Ctrl+Z/Y。
    // 回傳新建節點的 entity index(CreateNode)或 0(無效)。
    uint32_t CreateNode(Entity parent = Entity());
    void DestroyNode(Entity entity);
    void SetParent(Entity entity, Entity parent);
    void SetSelectedEntityMaterial(uint64_t materialId); // 對選中節點 push SetMaterialCommand

    // ---- Prefab(ADR-0001 D5,Undo/Redo 友善) ----
    // 將選中節點的子樹儲存為 Prefab 檔。回傳 true 成功。
    bool SavePrefab(uint32_t entityIndex, const String &path);
    // 從 Prefab 檔載入子樹到指定 parent 下(預設 root)。回傳新建根節點的 entity index,失敗回 0。
    uint32_t LoadPrefab(const String &path, Entity parent = Entity());
    // LoadPrefab 的別名(Instantiate)。
    uint32_t InstantiatePrefab(const String &path, Entity parent = Entity());

    // ---- Playback Control(ADR-0001 D4,SimRecorder 整合) ----
    // 播放控制(錄製/重播輸入)。EditorSession 只持有 SimRecorder* 指標,
    // 實際執行順序由 EngineContext/Sim 迴圈決定。
    // 播放中：Live 模式,每 tick 記錄輸入到 log。暫停後可重播。
    bool IsPlaying() const;
    bool IsReplaying() const;
    size_t GetPlaybackFrame() const;     // 目前 replay 游標(或 log 長度)
    size_t GetTotalFrames() const;       // log 長度
    // 開始播放(錄製):進入 Live 模式,清空舊 log,從頭開始記錄。
    void Play();
    // 暫停播放:停止記錄,保留 log。
    void Pause();
    // 停止播放:清空 log,回到停止狀態。
    void Stop();
    // 從頭重播:進入 Replay 模式,游標歸零。
    void Replay();

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

    // 建立預設 pane(Hierarchy LeftDock + Inspector RightDock)並串接:
    // Hierarchy 列點擊 → EditorSession::Select() → InspectorLayer::SetSelection()。
    // windowSize 由 App 提供(EditorSession 本身 window-agnostic)。
    void CreateDefaultPanes(const Point2D &windowSize);

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
    HashTable<uint64_t, String> mContentHashToPath;       // contentHash(materialId) → path
    DynamicArray<String> mTexturePaths;                   // 已載入貼圖路徑(依加載順序)
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