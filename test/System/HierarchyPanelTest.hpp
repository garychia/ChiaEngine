#ifndef HIERARCHY_PANEL_TEST_HPP
#define HIERARCHY_PANEL_TEST_HPP

#include "Data/String.hpp"
#include "Display/GUI/Border.hpp"
#include "Display/GUI/EditorSession.hpp"
#include "Display/GUI/HierarchyPanel.hpp"
#include "Display/GUI/InspectorLayer.hpp"
#include "Geometry/2D/Point2D.hpp"
#include "Math/Math.hpp"
#include "Scene/SceneSystem.hpp"
#include "System/World/Entity.hpp"
#include "Test.hpp"

#include <cstdint>

// HierarchyPanel + Inspector 選取同步(AC1):點擊 hierarchy 列 → EditorSession::Select()
// → InspectorLayer::SetSelection() 刷新欄位。全 headless。
namespace hierarchypanel_test
{

class HierarchyPanelTest : public Test
{
  public:
    HierarchyPanelTest() : Test("HierarchyPanel")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:建立節點樹,Rebuild 產生對應行數 ----
        {
            SceneSystem system;
            // root(0) -> child1(1) -> grandchild(2); root(0) -> child2(3)
            Entity root = system.CreateNode();
            Entity child1 = system.CreateNode(root);
            Entity grandchild = system.CreateNode(child1);
            Entity child2 = system.CreateNode(root);

            UndoStack stack;
            EditorSession session(&system, nullptr, nullptr);
            InspectorLayer inspector(String(u"Inspector"), Point2D(1000.f, 800.f),
                                     Border(800.f, 30.f, 200.f, 400.f), PanelDock::RightDock,
                                     &system, &stack);
            HierarchyPanel panel(String(u"Hierarchy"), Point2D(1000.f, 800.f),
                                 Border(0.f, 0.f, 240.f, 800.f), PanelDock::LeftDock,
                                 &system, &session, &inspector);

            panel.Rebuild();
            EXPECT_TRUE(panel.GetNRows() == 4, "Rebuild 產生 4 個節點行", true);

            // 每行標籤為節點名稱(預設 "Node")
            for (uint32_t i = 0; i < panel.GetNRows(); i++)
            {
                const String &label = panel.GetRow(i)->GetLabel();
                EXPECT_TRUE(label.Length() > 0, "每行標籤非空白", true);
            }
        }

        // ---- AC2:點擊列 → EditorSession 選取更新 + Inspector 欄位刷新 ----
        {
            SceneSystem system;
            Entity root = system.CreateNode();
            Entity child = system.CreateNode(root);
            system.GetLocalTransform(child)->position = Point3D(1.f, 2.f, 3.f);

            UndoStack stack;
            EditorSession session(&system, nullptr, nullptr);
            InspectorLayer inspector(String(u"Inspector"), Point2D(1000.f, 800.f),
                                     Border(800.f, 30.f, 200.f, 400.f), PanelDock::RightDock,
                                     &system, &stack);
            HierarchyPanel panel(String(u"Hierarchy"), Point2D(1000.f, 800.f),
                                 Border(0.f, 0.f, 240.f, 800.f), PanelDock::LeftDock,
                                 &system, &session, &inspector);

            panel.Rebuild();

            // 點擊 child 行(index 1)
            panel.GetRow(1)->rowClicked.Invoke(child);

            EXPECT_TRUE(session.GetSelection().hasSelection, "Select 後 hasSelection 為 true", true);
            EXPECT_TRUE(session.GetSelection().entityIndex == child.GetIndex(),
                        "Select 正確 entity index", true);

            // Inspector Pos X 欄應有顯示(SetSelection 被呼叫 → RebuildValueLabels)
            const String &posXLabel = inspector.GetFieldRows()[0]->GetLabel();
            EXPECT_TRUE(posXLabel.Length() > 0, "Inspector Pos X 欄有顯示文字", true);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }
};

} // namespace hierarchypanel_test

#endif // HIERARCHY_PANEL_TEST_HPP
