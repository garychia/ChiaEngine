#ifndef ASSET_BROWSER_PANEL_HPP
#define ASSET_BROWSER_PANEL_HPP

#include "Data/DynamicArray.hpp"
#include "Data/String.hpp"
#include "Display/GUI/Button.hpp"
#include "Display/GUI/PanelPane.hpp"

class EditorSession;
class SceneSystem;

// Asset Browser(LeftDock named pane):列出 EditorSession 已載入的貼圖(依
// GetTextureCount/GetTexturePathAt),每個貼圖一個按鈕。點擊貼圖按鈕 →
// EditorSession::BindTextureToSelectedEntity(path) 將 materialId 綁定到
// 目前選取的場景節點。無選取節點或無貼圖時顯示空列表/按鈕 no-op。
// 邏輯全在 Sim/View 純資料層,可無頭測試。
class AssetBrowserPanel : public PanelPane
{
  public:
    // pSession:供查詢已載入貼圖列表與綁定材質(不擁有,可為 nullptr)。
    AssetBrowserPanel(const String &title, const Point2D &windowSize, const Border &border,
                      PanelDock dock, SceneSystem *pScene, EditorSession *pSession);

    // 每幀由 Panel::Render 呼叫:若已載入貼圖列表變動則重建按鈕。
    void Update();

    // 測試用:取貼圖按鈕數量(=已載入貼圖數)。
    size_t GetTextureButtonCount() const { return mTextureButtons.GetNElements(); }

    // 測試用:取第 index 個貼圖按鈕的標籤(貼圖路徑)。
    const String &GetTextureButtonLabel(size_t index) const;

  private:
    // 每個貼圖條目:按鈕 + 路徑 + 自身作為 Event subscriber(避免 Event<void()>
    // 不傳參數且按 subscriber 去重的限制)。
    struct TextureEntry
    {
        AssetBrowserPanel *mpPanel;
        String path;
        SharedPtr<Button> button;

        TextureEntry(AssetBrowserPanel *pPanel, const String &pPath)
            : mpPanel(pPanel), path(pPath)
        {
        }

        void OnPressed()
        {
            mpPanel->BindPath(path);
        }
    };

    SceneSystem *mpSceneSystem;
    EditorSession *mpSession;

    DynamicArray<SharedPtr<TextureEntry>> mTextureEntries; // 每個已載入貼圖一個條目
    DynamicArray<SharedPtr<Button>> mTextureButtons;       // 對應按鈕(僅供測試查詢標籤)

    void Rebuild(); // 根據 EditorSession 的貼圖列表重建按鈕
    void BindPath(const String &path); // 將 path 綁定到選取節點
};

#endif // ASSET_BROWSER_PANEL_HPP
