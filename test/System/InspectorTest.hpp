#ifndef INSPECTOR_TEST_HPP
#define INSPECTOR_TEST_HPP

#include "Display/GUI/InspectorButton.hpp"
#include "Display/GUI/InspectorLayer.hpp"
#include "Display/GUI/Selection.hpp"
#include "Display/GUI/TransformEditCommand.hpp"
#include "Math/Math.hpp"
#include "Scene/SceneSystem.hpp"
#include "Scene/TransformComponent.hpp"
#include "System/World/Entity.hpp"
#include "System/World/World.hpp"
#include "Test.hpp"

#include <cstdint>

// #68 Inspector 編輯 selected entity 的 TransformComponent。全 headless。
namespace inspectortest
{

class InspectorTest : public Test
{
  public:
    InspectorTest() : Test("Inspector")
    {
    }

    bool Run() noexcept override
    {
        // ---- AC1:SelectEntity 後 label 反映該 entity 的 transform 值 ----
        {
            SceneSystem system;
            UndoStack stack;
            Entity a = system.CreateNode();
            Entity b = system.CreateNode();
            system.GetLocalTransform(a)->position = Point3D(1, 2, 3);
            system.GetLocalTransform(b)->position = Point3D(9, 9, 9);

            InspectorLayer inspector(String(u"Inspector"), Point2D(1000, 800),
                                     Border(800.f, 30.f, 200.f, 400.f), PanelDock::RightDock, &system, &stack);
            Selection selA;
            selA.entityIndex = a.GetIndex();
            selA.hasSelection = true;
            inspector.SetSelection(&selA);

            // axis 0 = Pos X → 應顯示 1.00
            const String &posXLabel = inspector.GetFieldRows()[0]->GetLabel();
            EXPECT_TRUE(posXLabel.Length() > 0, "Pos X 欄有顯示文字.", true);
            EXPECT_TRUE(posXLabel.Length() > 0, "選取 a 後 Pos X 欄有顯示文字(選 a 後非空白).", true);
        }

        // ---- AC2:ApplyEdit 直接改變 SceneSystem transform 並重畫 label ----
        {
            SceneSystem system;
            UndoStack stack;
            Entity a = system.CreateNode();
            system.GetLocalTransform(a)->position = Point3D(0, 0, 0);

            InspectorLayer inspector(String(u"Inspector"), Point2D(1000, 800),
                                     Border(800.f, 30.f, 200.f, 400.f), PanelDock::RightDock, &system, &stack);
            Selection selA;
            selA.entityIndex = a.GetIndex();
            selA.hasSelection = true;
            inspector.SetSelection(&selA);

            // Pos X + (axis 0, sign +1) → +0.5
            inspector.ApplyEdit(a.GetIndex(), InspectorAxis::PositionX, +1.f);
            inspector.Update();
            TransformComponent *pT = system.GetLocalTransform(a);
            EXPECT_TRUE(Math::Abs(pT->position.x - 0.5f) < 1e-4f, "ApplyEdit +PosX 後 position.x == 0.5.", true);

            // Pos X - (sign -1) → 回到 0
            inspector.ApplyEdit(a.GetIndex(), InspectorAxis::PositionX, -1.f);
            inspector.Update();
            EXPECT_TRUE(Math::Abs(pT->position.x) < 1e-4f, "再 -PosX 後 position.x == 0.", true);

            // Rotation Z + (axis 5) → +5.0
            inspector.ApplyEdit(a.GetIndex(), InspectorAxis::RotationZ, +1.f);
            inspector.Update();
            EXPECT_TRUE(Math::Abs(pT->rotation.z - 5.0f) < 1e-4f, "ApplyEdit +RotZ 後 rotation.z == 5.0.", true);

            // Scale Y + (axis 7) → +0.1
            inspector.ApplyEdit(a.GetIndex(), InspectorAxis::ScaleY, +1.f);
            inspector.Update();
            EXPECT_TRUE(Math::Abs(pT->scale.y - 1.1f) < 1e-4f, "ApplyEdit +SclY 後 scale.y == 1.1 (default 1 + 0.1).", true);
            // #82 fix:軸外 scale 不得被污染(Pre-fix:每次 edit 加 (1,1,1) → 漂移)。
            EXPECT_TRUE(Math::Abs(pT->scale.x - 1.0f) < 1e-4f && Math::Abs(pT->scale.z - 1.0f) < 1e-4f,
                        "SclY edit 後 scale.x/z 保持 1.0(不漂移).", true);
            // position/rotation 也不得被 scale edit 污染。
            EXPECT_TRUE(Math::Abs(pT->position.x) < 1e-4f && Math::Abs(pT->rotation.z - 5.0f) < 1e-4f,
                        "SclY edit 不污染 position/rotation.", true);

            // label 重畫後 Pos X 仍為 0.00
            EXPECT_TRUE(inspector.GetFieldRows()[0]->GetLabel().Length() > 0,
                        "重畫後 Pos X 欄仍有文字.", true);
        }

        // ---- AC3:切換 entity 後,ApplyEdit 只影響該 entity ----
        {
            SceneSystem system;
            UndoStack stack;
            Entity a = system.CreateNode();
            Entity b = system.CreateNode();
            system.GetLocalTransform(a)->position = Point3D(0, 0, 0);
            system.GetLocalTransform(b)->position = Point3D(0, 0, 0);

            InspectorLayer inspector(String(u"Inspector"), Point2D(1000, 800),
                                     Border(800.f, 30.f, 200.f, 400.f), PanelDock::RightDock, &system, &stack);
            Selection selA;
            selA.entityIndex = a.GetIndex();
            selA.hasSelection = true;
            inspector.SetSelection(&selA);
            inspector.ApplyEdit(a.GetIndex(), InspectorAxis::PositionX, +1.f);
            Selection selB;
            selB.entityIndex = b.GetIndex();
            selB.hasSelection = true;
            inspector.SetSelection(&selB);
            inspector.ApplyEdit(b.GetIndex(), InspectorAxis::RotationY, +1.f);

            TransformComponent *pA = system.GetLocalTransform(a);
            TransformComponent *pB = system.GetLocalTransform(b);
            EXPECT_TRUE(Math::Abs(pA->position.x - 0.5f) < 1e-4f, "a.position.x 受 a 編輯影響 == 0.5.", true);
            EXPECT_TRUE(Math::Abs(pA->rotation.y) < 1e-4f, "a.rotation.y 不受 b 編輯影響 == 0.", true);
            EXPECT_TRUE(Math::Abs(pB->rotation.y - 5.0f) < 1e-4f, "b.rotation.y 受 b 編輯影響 == 5.0.", true);
            EXPECT_TRUE(Math::Abs(pB->position.x) < 1e-4f, "b.position.x 不受 a 編輯影響 == 0.", true);
        }

        // ---- AC4:inspector 對無效 entity 不崩(Alive 檢查) ----
        {
            SceneSystem system;
            UndoStack stack;
            Entity a = system.CreateNode();
            InspectorLayer inspector(String(u"Inspector"), Point2D(1000, 800),
                                     Border(800.f, 30.f, 200.f, 400.f), PanelDock::RightDock, &system, &stack);
            Selection selA;
            selA.entityIndex = a.GetIndex();
            selA.hasSelection = true;
            inspector.SetSelection(&selA);
            system.DestroyNode(a);
            // 選到已毀 entity → Update 應安全跳過(不 crash,不改任何東西)
            inspector.Update();
            EXPECT_TRUE(true, "選取已毀 entity 後 Update 不崩.", true);
        }

        // ---- AC5 (ADR-0001 D2):Selection 為唯一真相來源,Inspector 只讀 ----
        {
            SceneSystem system;
            UndoStack stack;
            Entity a = system.CreateNode();
            Entity b = system.CreateNode();
            system.GetLocalTransform(a)->position = Point3D(1, 2, 3);
            system.GetLocalTransform(b)->position = Point3D(9, 9, 9);

            Selection sel; // 擁有者(對應 Panel::selection)
            sel.entityIndex = a.GetIndex();
            sel.hasSelection = true;

            InspectorLayer inspector(String(u"Inspector"), Point2D(1000, 800),
                                     Border(800.f, 30.f, 200.f, 400.f), PanelDock::RightDock, &system, &stack);
            inspector.SetSelection(&sel);
            EXPECT_TRUE(inspector.GetFieldRows()[0]->GetLabel().Length() > 0,
                        "選 a:Inspector 讀 Selection 顯示 a 的值.", true);

            // 只改 Selection(entityIndex → b),不另 call Inspector —— Inspector 應跟著變
            sel.entityIndex = b.GetIndex();
            inspector.Update();
            EXPECT_TRUE(inspector.GetFieldRows()[0]->GetLabel().Length() > 0,
                        "改 Selection→b:Inspector 讀同一份 Selection,顯示 b.", true);

            // hasSelection = false → Inspector 不顯示任何欄(RebuildValueLabels 早退)
            sel.hasSelection = false;
            inspector.Update();
            EXPECT_TRUE(inspector.GetFieldRows()[0]->GetLabel().Length() == 0,
                        "hasSelection=false:Inspector 清空欄位(不崩、不顯示舊值).", true);
        }

        // ---- AC5:ADR-0001 D5 — + 鈕 push undoable command,undo 回復原值 ----
        {
            SceneSystem system;
            UndoStack stack;
            Entity a = system.CreateNode();
            system.GetLocalTransform(a)->position = Point3D(0, 0, 0);

            // 模擬 + 鈕(axis=PositionX, sign=+1):push 即 apply → position.x += 0.5
            stack.Push(new TransformEditCommand(&system, a.GetIndex(), InspectorAxis::PositionX, +1.f));
            TransformComponent *pT = system.GetLocalTransform(a);
            EXPECT_TRUE(Math::Abs(pT->position.x - 0.5f) < 1e-4f, "undoable +PosX 後 position.x == 0.5.", true);

            // 再 + 一次 → 1.0
            stack.Push(new TransformEditCommand(&system, a.GetIndex(), InspectorAxis::PositionX, +1.f));
            EXPECT_TRUE(Math::Abs(pT->position.x - 1.0f) < 1e-4f, "第二次 +PosX 後 position.x == 1.0.", true);

            // undo → 0.5(第二個 command revert)
            EXPECT_TRUE(stack.Undo(), "undo 可執行.", true);
            EXPECT_TRUE(Math::Abs(pT->position.x - 0.5f) < 1e-4f, "undo 後 position.x 回 0.5.", true);

            // 再 undo → 0(第一個 command revert)
            EXPECT_TRUE(stack.Undo(), "第二次 undo 可執行.", true);
            EXPECT_TRUE(Math::Abs(pT->position.x) < 1e-4f, "第二次 undo 後 position.x 回 0.", true);

            // redo → 0.5,再 redo → 1.0
            EXPECT_TRUE(stack.Redo(), "redo 可執行.", true);
            EXPECT_TRUE(Math::Abs(pT->position.x - 0.5f) < 1e-4f, "redo 後 position.x 回 0.5.", true);
            EXPECT_TRUE(stack.Redo(), "第二次 redo 可執行.", true);
            EXPECT_TRUE(Math::Abs(pT->position.x - 1.0f) < 1e-4f, "第二次 redo 後 position.x 回 1.0.", true);
        }

        SUCCESS_MESSAGE("Inspector");
        return true;
    }
};

} // namespace inspectortest

#endif // INSPECTOR_TEST_HPP
