#ifndef ASSET_BROWSER_TEST_HPP
#define ASSET_BROWSER_TEST_HPP

#include "Data/String.hpp"
#include "Display/GUI/AssetBrowserPanel.hpp"
#include "Display/GUI/EditorSession.hpp"
#include "Display/GUI/InspectorLayer.hpp"
#include "Geometry/2D/Point2D.hpp"
#include "Scene/SceneSystem.hpp"
#include "System/Asset/AssetManager.hpp"
#include "System/Job/JobSystem.hpp"
#include "Test.hpp"

#include <cstdint>
#include <fstream>

// Asset Browser(#168 優先級 3):已載入貼圖列表 + 點擊貼圖綁定到選中節點。
// 全 headless,使用人造 PPM P6 貼圖(與 EditorSessionTest 一致,無跨平台差異)。
namespace assetbrowser_test
{

// Mock renderer asset registrar:記錄 RegisterMaterial 呼叫。
class MockRegistrar : public IRendererAssetRegistrar
{
  public:
    bool registerCalled = false;
    uint64_t lastMaterialId = 0;
    int lastWidth = 0;
    int lastHeight = 0;
    bool lastHadRawRGBA = false;

    bool RegisterMeshGeometry(uint64_t meshId, const RenderInfo &info) override { return false; }

    bool RegisterMaterial(uint64_t materialId, const MaterialSource &source) override
    {
        registerCalled = true;
        lastMaterialId = materialId;
        lastWidth = source.width;
        lastHeight = source.height;
        lastHadRawRGBA = (source.pRawRGBA != nullptr);
        return true;
    }
};

class AssetBrowserTest : public Test
{
  public:
    AssetBrowserTest() : Test("AssetBrowser")
    {
    }

    static void WriteTestPpm(const char *path, int w, int h,
                             unsigned char r, unsigned char g, unsigned char b)
    {
        std::ofstream file(path, std::ios::binary);
        file << "P6\n" << w << " " << h << "\n255\n";
        for (int i = 0; i < w * h; ++i)
        {
            file.put(r);
            file.put(g);
            file.put(b);
        }
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:無貼圖時 AssetBrowser 顯示 0 個按鈕 ----
        {
            SceneSystem system;
            EditorSession session(&system, nullptr, nullptr);
            AssetBrowserPanel browser(String(u"AssetBrowser"), Point2D(1000.f, 800.f),
                                      Border(0.f, 0.f, 240.f, 800.f), PanelDock::LeftDock,
                                      &system, &session);
            browser.Update();
            EXPECT_TRUE(browser.GetTextureButtonCount() == 0,
                        "無載入貼圖 → GetTextureButtonCount 為 0", true);
            EXPECT_TRUE(browser.GetTextureButtonLabel(0).Length() == 0,
                        "無貼圖 → GetTextureButtonLabel(0) 回空字串", true);
        }

        // ---- AC2:載入一張貼圖後 AssetBrowser 顯示 1 個按鈕,標籤為路徑 ----
        {
            JobSystem jobs(2);
            AssetManager manager(jobs);
            MockRegistrar mockReg;
            SceneSystem system;
            EditorSession session(&system, nullptr, nullptr, &manager, &mockReg);

            const char *path = TestTempFilePath("chia_assetbrowser_tex.ppm");
            WriteTestPpm(path, 2, 1, 10, 20, 30);

            session.LoadTexture(String(path));
            jobs.WaitForIdle();
            session.DispatchAssetEvents();

            AssetBrowserPanel browser(String(u"AssetBrowser"), Point2D(1000.f, 800.f),
                                      Border(0.f, 0.f, 240.f, 800.f), PanelDock::LeftDock,
                                      &system, &session);
            browser.Update();
            EXPECT_TRUE(browser.GetTextureButtonCount() == 1,
                        "載入 1 張貼圖 → GetTextureButtonCount 為 1", true);
            EXPECT_TRUE(browser.GetTextureButtonLabel(0) == String(path),
                        "按鈕標籤為貼圖路徑", true);

            std::remove(path);
        }

        // ---- AC3:點擊貼圖按鈕(模擬) → 綁定到選中節點 → Inspector 顯示 "Material: <id>" ----
        {
            JobSystem jobs(2);
            AssetManager manager(jobs);
            MockRegistrar mockReg;
            SceneSystem system;
            Entity root = system.CreateNode();
            EditorSession session(&system, nullptr, nullptr, &manager, &mockReg);
            session.Select(root.GetIndex());

            const char *path = TestTempFilePath("chia_assetbrowser_bind.ppm");
            WriteTestPpm(path, 2, 1, 10, 20, 30);

            session.LoadTexture(String(path));
            jobs.WaitForIdle();
            session.DispatchAssetEvents();

            // 確認貼圖已載入(EditorSession 記錄 path → contentHash)
            uint64_t materialId = session.GetMaterialId(String(path));
            EXPECT_TRUE(materialId != 0, "GetMaterialId 回傳非零", true);

            // 點擊貼圖按鈕(透過 EditorSession 直接綁定,模擬 AssetBrowser 點擊行為)
            session.BindTextureToSelectedEntity(String(path));
            EXPECT_TRUE(session.GetSelectedEntityMaterialId() == materialId,
                        "綁定後選中節點 materialId == 貼圖 contentHash", true);

            // Inspector 顯示 "Material: <id>"
            UndoStack stack;
            InspectorLayer inspector(String(u"Inspector"), Point2D(1000.f, 800.f),
                                     Border(800.f, 30.f, 200.f, 400.f), PanelDock::RightDock,
                                     &system, &stack, &session);
            Selection sel;
            sel.entityIndex = root.GetIndex();
            sel.hasSelection = true;
            inspector.SetSelection(&sel);
            inspector.Update();

            String expected = String(u"Material: ");
            expected = expected + String(Str<char16_t>::FromInt(materialId));
            EXPECT_TRUE(inspector.GetMaterialLabel() == expected,
                        "Inspector 顯示 Material: <id>", true);

            std::remove(path);
        }

        // ---- AC4:無選中節點時點擊貼圖 → 無操作(no-op) ----
        {
            JobSystem jobs(2);
            AssetManager manager(jobs);
            MockRegistrar mockReg;
            SceneSystem system;
            EditorSession session(&system, nullptr, nullptr, &manager, &mockReg);

            const char *path = TestTempFilePath("chia_assetbrowser_noselect.ppm");
            WriteTestPpm(path, 2, 1, 10, 20, 30);

            session.LoadTexture(String(path));
            jobs.WaitForIdle();
            session.DispatchAssetEvents();

            // 無選中節點時綁定 → no-op
            session.BindTextureToSelectedEntity(String(path));
            EXPECT_TRUE(session.GetSelectedEntityMaterialId() == 0,
                        "無選中節點 → GetSelectedEntityMaterialId 為 0", true);

            std::remove(path);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }
};

} // namespace assetbrowser_test

#endif // ASSET_BROWSER_TEST_HPP
