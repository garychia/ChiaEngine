#include "Display/GUI/AssetBrowserPanel.hpp"
#include "Display/GUI/EditorSession.hpp" // GetTextureCount/GetTexturePathAt/BindTextureToSelectedEntity
#include "Scene/SceneSystem.hpp"

AssetBrowserPanel::AssetBrowserPanel(const String &title, const Point2D &windowSize,
                                     const Border &border, PanelDock dock,
                                     SceneSystem *pScene, EditorSession *pSession)
    : PanelPane(title, windowSize, border, dock), mpSceneSystem(pScene), mpSession(pSession)
{
    SetColor(Color(0.10f, 0.10f, 0.12f));
    Rebuild();
}

void AssetBrowserPanel::Update()
{
    Rebuild();
}

const String &AssetBrowserPanel::GetTextureButtonLabel(size_t index) const
{
    // 調用者需保證 index < GetTextureButtonCount();越界回空字串(不崩)。
    if (index >= mTextureButtons.GetNElements())
    {
        static const String empty;
        return empty;
    }
    return mTextureButtons[index]->GetLabel();
}

void AssetBrowserPanel::Rebuild()
{
    if (!mpSession)
        return;

    const size_t nTextures = mpSession->GetTextureCount();
    // 當條目數與貼圖數一致時不重建(避免每幀 churn)。
    if (mTextureEntries.GetNElements() == nTextures && nTextures > 0)
        return;

    // 移除舊按鈕與條目(PanelPane::RemoveComponents 會清空 p_components)。
    RemoveComponents();
    mTextureEntries.RemoveAll();
    mTextureButtons.RemoveAll();

    const float rowHeight = 22.f;
    for (size_t i = 0; i < nTextures; i++)
    {
        const String &path = mpSession->GetTexturePathAt(i);
        const float y = static_cast<float>(i) * rowHeight + 4.f;

        auto pEntry = SharedPtr<TextureEntry>::Construct(this, path);
        auto pBtn = AddComponent<Button>(
            /*windowSize=*/Point2D(), /*border=*/Border(4.f, y, 200.f, rowHeight - 2.f));
        pBtn->SetLabel(path);
        pBtn->SetFontSize(10.f);
        pBtn->pressEvent.Subscribe(pEntry.GetRaw(), &TextureEntry::OnPressed);
        pEntry->button = pBtn;
        mTextureEntries.Append(pEntry);
        mTextureButtons.Append(pBtn);
    }
}

void AssetBrowserPanel::BindPath(const String &path)
{
    if (!mpSession || !mpSceneSystem)
        return;
    mpSession->BindTextureToSelectedEntity(path);
}
