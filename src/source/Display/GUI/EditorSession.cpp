#include "Display/GUI/EditorSession.hpp"
#include "Display/GUI/AssetBrowserPanel.hpp"
#include "Display/GUI/HierarchyPanel.hpp"
#include "Display/GUI/InspectorLayer.hpp"
#include "Display/GUI/SceneEditCommand.hpp"
#include "Geometry/2D/Point2D.hpp"
#include "Display/GUI/Border.hpp"

EditorSession::EditorSession(SceneSystem *pSceneSystem, CameraController *pCameraController,
                             SimRecorder *pSimRecorder, AssetManager *pAssetManager,
                             IRendererAssetRegistrar *pRegistrar)
    : mpSceneSystem(pSceneSystem), mpCameraController(pCameraController), mpSimRecorder(pSimRecorder),
      mpAssetManager(pAssetManager), mpRegistrar(pRegistrar), mTextureView(),
      mTextureToContentHash(), mSelection(), mUndoStack(), mPanes()
{
    // 綁定 AssetManager:TextureAssetView 訂閱 LoadedEvent(解碼 → 轉交 hook);
    // EditorSession 自身也訂閱,以記錄 path → contentHash 映射(供 GetMaterialId 查詢)。
    if (mpAssetManager)
    {
        mTextureView.SubscribeTo(*mpAssetManager);
        mpAssetManager->LoadedEvent.Subscribe(this, &EditorSession::OnAssetLoaded);
    }

    // 綁定 renderer 註冊器:解碼完成後的 hook 會把 RGBA pixels 經 RegisterMaterial 上傳 GPU。
    if (mpRegistrar)
        mTextureView.SetTextureReadyHook(&EditorSession::OnTextureReadyStatic, this);
}

EditorSession::~EditorSession()
{
    if (mpAssetManager)
        mpAssetManager->LoadedEvent.Unsubscribe(this);
    mTextureView.Unsubscribe();
    mUndoStack.Clear();
}

void EditorSession::SetAssetManager(AssetManager *pAssetManager)
{
    if (mpAssetManager == pAssetManager)
        return;
    if (mpAssetManager)
    {
        mpAssetManager->LoadedEvent.Unsubscribe(this);
        mTextureView.Unsubscribe();
    }
    mpAssetManager = pAssetManager;
    if (mpAssetManager)
    {
        mTextureView.SubscribeTo(*mpAssetManager);
        mpAssetManager->LoadedEvent.Subscribe(this, &EditorSession::OnAssetLoaded);
    }
}

void EditorSession::SetAssetRegistrar(IRendererAssetRegistrar *pRegistrar)
{
    mpRegistrar = pRegistrar;
    if (mpRegistrar)
        mTextureView.SetTextureReadyHook(&EditorSession::OnTextureReadyStatic, this);
}

uint64_t EditorSession::LoadTexture(const String &path)
{
    if (!mpAssetManager)
        return 0;
    AssetManager::AssetId id = mpAssetManager->LoadAsync(path);
    (void)id; // AssetId 由 AssetManager/TextureAssetView 管理,EditorSession 僅追蹤 pathHash
    return HashPath(path);
}

void EditorSession::DispatchAssetEvents()
{
    if (mpAssetManager)
        mpAssetManager->DispatchCompletedEvents();
}

uint64_t EditorSession::GetMaterialId(const String &path) const
{
    uint64_t pathHash = HashPath(path);
    typename HashTable<uint64_t, uint64_t>::Iterator itr = mTextureToContentHash.Find(pathHash);
    if (itr != mTextureToContentHash.Last())
        return itr->Value();
    return 0;
}

bool EditorSession::IsTextureLoaded(const String &path) const
{
    return GetMaterialId(path) != 0;
}

void EditorSession::OnAssetLoaded(SharedPtr<Asset> asset)
{
    if (!asset || !asset->loaded || !asset->pBlock)
        return;
    uint64_t pathHash = HashPath(asset->key);
    uint64_t contentHash = asset->ContentHash();
    mTextureToContentHash.Insert(pathHash, contentHash);
    mContentHashToPath.Insert(contentHash, asset->key);
    // 記錄路徑順序(去重:同一 path 只會觸發一次 LoadedEvent)。
    bool found = false;
    for (size_t i = 0; i < mTexturePaths.GetNElements(); i++)
    {
        if (mTexturePaths[i] == asset->key)
        {
            found = true;
            break;
        }
    }
    if (!found)
        mTexturePaths.Append(asset->key);
}

void EditorSession::OnTextureReadyStatic(const unsigned char *rgbaPixels, int width, int height,
                                         size_t contentHash, void *pUserData)
{
    EditorSession *self = static_cast<EditorSession *>(pUserData);
    if (!self || !self->mpRegistrar || !rgbaPixels)
        return;
    MaterialSource src{};
    src.pRawRGBA = rgbaPixels;
    src.width = static_cast<uint32_t>(width);
    src.height = static_cast<uint32_t>(height);
    self->mpRegistrar->RegisterMaterial(static_cast<uint64_t>(contentHash), src);
}

uint64_t EditorSession::HashPath(const String &path)
{
    const Str<char> utf8 = path.ToUTF8();
    return FNV1A64(FNV1A64_OFFSET, utf8.CStr(), utf8.Length());
}

size_t EditorSession::GetTextureCount() const
{
    return mTexturePaths.GetNElements();
}

const String &EditorSession::GetTexturePathAt(size_t index) const
{
    // 調用者需保證 index < GetTextureCount();越界回空字串(不崩)。
    if (index >= mTexturePaths.GetNElements())
    {
        static const String empty;
        return empty;
    }
    return mTexturePaths[index];
}

void EditorSession::BindTextureToSelectedEntity(const String &path)
{
    if (!mpSceneSystem || !mSelection.hasSelection)
        return;
    uint64_t materialId = GetMaterialId(path);
    mpSceneSystem->SetMaterial(mSelection.entityIndex, materialId);
}

uint64_t EditorSession::GetSelectedEntityMaterialId() const
{
    if (!mpSceneSystem || !mSelection.hasSelection)
        return 0;
    uint64_t materialId = 0;
    mpSceneSystem->GetMaterial(mSelection.entityIndex, materialId);
    return materialId;
}

uint32_t EditorSession::CreateNode(Entity parent)
{
    if (!mpSceneSystem)
        return 0;
    // UndoStack::Push 會立即呼叫 cmd->Apply(),由 CreateNodeCommand 建立節點
    // 並記錄 mEntityIndex;Push 後即可取得新節點 index。
    auto *cmd = new CreateNodeCommand(mpSceneSystem, parent);
    mUndoStack.Push(cmd);
    return cmd->GetCreatedEntityIndex();
}

void EditorSession::DestroyNode(Entity entity)
{
    if (!mpSceneSystem)
        return;
    mUndoStack.Push(new DestroyNodeCommand(mpSceneSystem, entity));
}

void EditorSession::SetParent(Entity entity, Entity parent)
{
    if (!mpSceneSystem)
        return;
    mUndoStack.Push(new SetParentCommand(mpSceneSystem, entity, parent));
}

void EditorSession::SetSelectedEntityMaterial(uint64_t materialId)
{
    if (!mpSceneSystem || !mSelection.hasSelection)
        return;
    mUndoStack.Push(new SetMaterialCommand(mpSceneSystem, mSelection.entityIndex, materialId));
}

bool EditorSession::SavePrefab(uint32_t entityIndex, const String &path)
{
    if (!mpSceneSystem)
        return false;
    Entity e = mpSceneSystem->GetEntityByIndex(entityIndex);
    if (!mpSceneSystem->Alive(e))
        return false;
    const Str<char> utf8 = path.ToUTF8();
    return mpSceneSystem->SavePrefab(e, utf8.CStr());
}

uint32_t EditorSession::LoadPrefab(const String &path, Entity parent)
{
    if (!mpSceneSystem)
        return 0;
    const Str<char> utf8 = path.ToUTF8();
    Entity e = mpSceneSystem->LoadPrefab(parent, utf8.CStr());
    return e.GetIndex();
}

uint32_t EditorSession::InstantiatePrefab(const String &path, Entity parent)
{
    return LoadPrefab(path, parent);
}

bool EditorSession::IsPlaying() const
{
    return mpSimRecorder != nullptr && !mpSimRecorder->IsReplaying() &&
           mpSimRecorder->GetInputCount() > 0;
}

bool EditorSession::IsReplaying() const
{
    return mpSimRecorder != nullptr && mpSimRecorder->IsReplaying();
}

size_t EditorSession::GetPlaybackFrame() const
{
    if (!mpSimRecorder)
        return 0;
    return mpSimRecorder->IsReplaying() ? mpSimRecorder->GetInputCount() : 0;
}

size_t EditorSession::GetTotalFrames() const
{
    if (!mpSimRecorder)
        return 0;
    return mpSimRecorder->GetInputCount();
}

void EditorSession::Play()
{
    if (!mpSimRecorder)
        return;
    mpSimRecorder->ClearLog();
    mpSimRecorder->SetReplaying(false);
    // Live 模式下,FixedUpdate 會自動 Append input 到 log
}

void EditorSession::Pause()
{
    if (!mpSimRecorder)
        return;
    mpSimRecorder->SetReplaying(false);
}

void EditorSession::Stop()
{
    if (!mpSimRecorder)
        return;
    mpSimRecorder->SetReplaying(false);
    mpSimRecorder->ClearLog();
}

void EditorSession::Replay()
{
    if (!mpSimRecorder)
        return;
    mpSimRecorder->BeginReplay();
}

void EditorSession::CreateDefaultPanes(const Point2D &windowSize)
{
    const Border leftBorder(0.f, 0.f, 240.f, windowSize.y);
    const Border rightBorder(0.f, 0.f, 320.f, windowSize.y);

    // Inspector(RightDock):TransformComponent + / - 編輯,Undoable。
    auto pInspector = SharedPtr<InspectorLayer>::Construct(
        String(u"Inspector"), windowSize, rightBorder, PanelDock::RightDock, mpSceneSystem,
        &mUndoStack, this);
    RegisterPane(pInspector);

    // Hierarchy(LeftDock):節點樹,點擊 → Select() → Inspector 刷新。
    auto pHierarchy = SharedPtr<HierarchyPanel>::Construct(
        String(u"Hierarchy"), windowSize, leftBorder, PanelDock::LeftDock, mpSceneSystem, this,
        pInspector.GetRaw());
    RegisterPane(pHierarchy);

    // Asset Browser(LeftDock):已載入貼圖列表,點擊貼圖 → 綁定到選中節點。
    auto pAssetBrowser = SharedPtr<AssetBrowserPanel>::Construct(
        String(u"AssetBrowser"), windowSize, leftBorder, PanelDock::LeftDock, mpSceneSystem, this);
    RegisterPane(pAssetBrowser);

    // Inspector 先同步目前選取(預設無選取)。
    if (mpSceneSystem)
        pInspector->SetSelection(&mSelection);

    // Wire up TransformGizmo now that InspectorLayer exists.
    mGizmo.Bind(mpSceneSystem, this, pInspector.GetRaw());
}

void EditorSession::Select(uint32_t entityIndex)
{
    mSelection.entityIndex = entityIndex;
    mSelection.hasSelection = true;
}

void EditorSession::ClearSelection()
{
    mSelection = Selection();
}

void EditorSession::HandleGizmoInput(float mouseX, float mouseY)
{
    if (!mGizmoActive || mGizmoSelectedEntity.GetRaw() == 0)
        return;

    // Update gizmo state based on mouse input
    GetGizmo().Update(mouseX, mouseY, 0.016f);  // 60fps frame time
}

void EditorSession::UpdateGizmoState()
{
    // This would be called from viewport render loop
    if (mGizmoActive && mGizmoSelectedEntity.GetRaw() != 0)
    {
        // Gizmo state is updated via TransformGizmo::Update from the render loop
        (void)0;
    }
}

void EditorSession::BeginGizmoManipulation(Entity entity, GizmoAxis axis)
{
    mGizmoSelectedEntity = entity;
    mGizmoActiveAxis = axis;
    mGizmoActive = true;

    // Start gizmo manipulation - select the entity in the hierarchy
    if (entity.GetRaw() != 0)
    {
        Select(entity.GetIndex());
    }
}

void EditorSession::EndGizmoManipulation()
{
    mGizmoActive = false;
    mGizmoSelectedEntity = Entity();
    mGizmoActiveAxis = GizmoAxis::Position;
}

void EditorSession::ApplyGizmoEdit()
{
    // Apply gizmo edits through the scene system
    if (mGizmoActive && mGizmoSelectedEntity.GetRaw() != 0)
    {
        // In a real implementation, we'd get the actual edits from the gizmo
        // and apply them through SceneSystem operations
        TransformComponent transform;
        if (mpSceneSystem && mpSceneSystem->GetTransform(mGizmoSelectedEntity.GetIndex(), transform))
        {
            // Simple example: translate along X axis based on gizmo state
            // This would be replaced with actual gizmo delta calculations
            transform.position.x += 0.1f;  // Simplified - real implementation would use proper delta
            mpSceneSystem->EditTransform(mGizmoSelectedEntity.GetIndex(), transform);
        }
    }
}

bool EditorSession::HandleEditorShortcut(bool hasCtrl, bool hasZ, bool hasY)
{
    if (hasCtrl && hasZ)
    {
        mUndoStack.Undo();
        return true;
    }
    if (hasCtrl && hasY)
    {
        mUndoStack.Redo();
        return true;
    }
    return false;
}

PanelPane *EditorSession::RegisterPane(SharedPtr<PanelPane> pPane)
{
    if (!pPane)
        return nullptr;
    if (PanelPane *pExisting = GetPane(pPane->GetTitle()))
        return pExisting;
    mPanes.Append(pPane);
    return pPane.GetRaw();
}

PanelPane *EditorSession::GetPane(const String &title)
{
    for (size_t i = 0; i < mPanes.GetNElements(); i++)
        if (mPanes[i]->GetTitle() == title)
            return mPanes[i].GetRaw();
    return nullptr;
}

void EditorSession::SetPaneCollapsed(const String &title, bool collapsed)
{
    if (PanelPane *pPane = GetPane(title))
        pPane->SetCollapsed(collapsed);
}

void EditorSession::Reset()
{
    mSelection = Selection();
    mUndoStack.Clear();
    mPanes.RemoveAll();
    mGizmoActive = false;
    mGizmoSelectedEntity = Entity();
    mGizmoActiveAxis = GizmoAxis::Position;
    mGizmo = TransformGizmo(mpSceneSystem, this, nullptr);
}
