#include "Display/GUI/EditorSession.hpp"

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
    mUndoStack.Clear();
}

void EditorSession::SetAssetManager(AssetManager *pAssetManager)
{
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
    mTextureToContentHash.Insert(pathHash, asset->ContentHash());
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

void EditorSession::Select(uint32_t entityIndex)
{
    mSelection.entityIndex = entityIndex;
    mSelection.hasSelection = true;
}

void EditorSession::ClearSelection()
{
    mSelection = Selection();
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
}
