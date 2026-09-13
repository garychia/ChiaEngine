#ifndef DOCK_REGISTRY_TEST_HPP
#define DOCK_REGISTRY_TEST_HPP

#include "Math/Math.hpp"
#include "Test.hpp"
#include "../../src/ChiaApp/Components/PanelLayout.hpp"

// #87 ADR-0001 D3:PanelLayout 成為 dock manager — named docks + PanelPane
// registry。驗證:
// - Toolbar / Hierarchy / Inspector 都以 named pane 註冊進 session(取代
//   GetLayers()[0] 魔術 index + 成員 slot 的手排)。
// - SetRegions 依每個 pane 的 dock assignment 定位(不再特判)。
// - collapsed pane 高度歸零;展開恢復。
// - 新面板 = 一個 RegisterPane 呼叫,之後即依 dock 定位。
// 純 GUI/Sim 資料層,無 GLFW Window → headless。
namespace dockregistrytest
{

class DockRegistryTest : public Test
{
  public:
    DockRegistryTest() : Test("DockRegistry")
    {
    }

    bool Run() noexcept override
    {
        const long W = 1000, H = 800;
        const PanelRegions regions = ComputePanelRegions(W, H, Point2D(4, 3));
        const Point2D ws(static_cast<float>(W), static_cast<float>(H));

        // ---- AC1:named docks 註冊 + SetRegions 依 dock 定位 ----
        {
            EditorSession session;
            PanelLayout layout(ws, session);

            EXPECT_TRUE(session.GetPane(String(u"Toolbar")) != nullptr, "ctor 註冊 Toolbar pane.", true);
            EXPECT_TRUE(session.GetPane(String(u"Toolbar"))->GetDock() == PanelDock::TopBar,
                        "Toolbar dock = TopBar.", true);

            SceneSystem scene;
            scene.CreateEditorDemoHierarchy();
            layout.BuildHierarchy(scene);
            layout.CreateInspector(scene);
            layout.SetRegions(ws, regions);

            PanelPane *pToolbar = session.GetPane(String(u"Toolbar"));
            EXPECT_TRUE(Near((float)pToolbar->GetBorder().xPos, regions.topBar.xPos) &&
                        Near((float)pToolbar->GetBorder().yPos, regions.topBar.yPos) &&
                        Near((float)pToolbar->GetBorder().width, regions.topBar.width) &&
                        Near((float)pToolbar->GetBorder().height, regions.topBar.height),
                        "Toolbar 依 topBar region 定位(取代 GetLayers()[0] 特判).", true);

            PanelPane *pHierarchy = session.GetPane(String(u"Hierarchy"));
            EXPECT_TRUE(pHierarchy != nullptr, "BuildHierarchy 註冊 Hierarchy pane.", true);
            EXPECT_TRUE(pHierarchy->GetDock() == PanelDock::LeftDock, "Hierarchy dock = LeftDock.", true);
            EXPECT_TRUE(Near((float)pHierarchy->GetBorder().xPos, regions.leftDock.xPos) &&
                        Near((float)pHierarchy->GetBorder().width, regions.leftDock.width) &&
                        Near((float)pHierarchy->GetBorder().height, regions.leftDock.height),
                        "Hierarchy 依 leftDock region 定位.", true);

            PanelPane *pInspector = session.GetPane(String(u"Inspector"));
            EXPECT_TRUE(pInspector != nullptr, "CreateInspector 註冊 Inspector pane.", true);
            EXPECT_TRUE(pInspector->GetDock() == PanelDock::RightDock, "Inspector dock = RightDock.", true);
            EXPECT_TRUE(Near((float)pInspector->GetBorder().xPos, regions.rightDock.xPos) &&
                        Near((float)pInspector->GetBorder().width, regions.rightDock.width) &&
                        Near((float)pInspector->GetBorder().height, regions.rightDock.height),
                        "Inspector 依 rightDock region 定位.", true);

            EXPECT_TRUE(layout.GetLayers().GetNElements() == 3, "dock manager 把 3 個 pane 加為 layers.", true);
        }

        // ---- AC2:collapsed pane 高度歸零;展開恢復 ----
        {
            EditorSession session;
            PanelLayout layout(ws, session);
            SceneSystem scene;
            scene.CreateEditorDemoHierarchy();
            layout.BuildHierarchy(scene);
            layout.CreateInspector(scene);

            session.SetPaneCollapsed(String(u"Inspector"), true);
            layout.SetRegions(ws, regions);
            EXPECT_TRUE((float)session.GetPane(String(u"Inspector"))->GetBorder().height < 1.f,
                        "collapsed Inspector 高度歸零.", true);

            session.SetPaneCollapsed(String(u"Inspector"), false);
            layout.SetRegions(ws, regions);
            EXPECT_TRUE(Near((float)session.GetPane(String(u"Inspector"))->GetBorder().height,
                             regions.rightDock.height),
                        "展開後恢復 rightDock 高度.", true);
        }

        // ---- AC3:新面板 = 一個 RegisterPane 呼叫,依 dock 定位(不用 SetRegions 手術)----
        {
            EditorSession session;
            PanelLayout layout(ws, session);
            layout.RegisterPane(
                SharedPtr<PanelPane>::Construct(String(u"Console"), ws, Border(), PanelDock::RightDock));
            layout.SetRegions(ws, regions);

            PanelPane *pConsole = session.GetPane(String(u"Console"));
            EXPECT_TRUE(pConsole != nullptr, "RegisterPane 加入新 pane.", true);
            EXPECT_TRUE(pConsole->GetTitle() == String(u"Console"), "pane 保留 title.", true);
            EXPECT_TRUE(Near((float)pConsole->GetBorder().xPos, regions.rightDock.xPos) &&
                        Near((float)pConsole->GetBorder().width, regions.rightDock.width),
                        "Console 依 rightDock 定位(不需 constructor + SetRegions 手術).", true);
            EXPECT_TRUE(layout.GetLayers().GetNElements() == 2, "新增 pane 加入 layers(Toolbar + Console).", true);
        }

        SUCCESS_MESSAGE("DockRegistry: named docks / PanelPane registry / SetRegions 定位 全部通過.");
        return true;
    }

  private:
    static bool Near(float a, float b)
    {
        const float d = a - b;
        return d < 1.f && d > -1.f;
    }
};

} // namespace dockregistrytest

#endif // DOCK_REGISTRY_TEST_HPP