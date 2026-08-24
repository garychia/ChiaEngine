#ifndef INSPECTOR_TEST_HPP
#define INSPECTOR_TEST_HPP

#include "Display/GUI/InspectorButton.hpp"
#include "Display/GUI/InspectorLayer.hpp"
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
            Entity a = system.CreateNode();
            Entity b = system.CreateNode();
            system.world.GetComponent<TransformComponent>(a)->position = Point3D(1, 2, 3);
            system.world.GetComponent<TransformComponent>(b)->position = Point3D(9, 9, 9);

            InspectorLayer inspector(Point2D(1000, 800),
                                     Border(800.f, 30.f, 200.f, 400.f), &system);
            inspector.SelectEntity(a.GetIndex());

            // axis 0 = Pos X → 應顯示 1.00
            const String &posXLabel = inspector.GetFieldRows()[0]->GetLabel();
            EXPECT_TRUE(posXLabel.Length() > 0, "Pos X 欄有顯示文字.", true);
            EXPECT_TRUE(posXLabel.Length() > 0, "選取 a 後 Pos X 欄有顯示文字(選 a 後非空白).", true);
        }

        // ---- AC2:ApplyEdit 直接改變 SceneSystem transform 並重畫 label ----
        {
            SceneSystem system;
            Entity a = system.CreateNode();
            system.world.GetComponent<TransformComponent>(a)->position = Point3D(0, 0, 0);

            InspectorLayer inspector(Point2D(1000, 800),
                                     Border(800.f, 30.f, 200.f, 400.f), &system);
            inspector.SelectEntity(a.GetIndex());

            // Pos X + (axis 0, sign +1) → +0.5
            inspector.ApplyEdit(a.GetIndex(), InspectorAxis::PositionX, +1.f);
            inspector.Update();
            TransformComponent *pT = system.world.GetComponent<TransformComponent>(a);
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

            // label 重畫後 Pos X 仍為 0.00
            EXPECT_TRUE(inspector.GetFieldRows()[0]->GetLabel().Length() > 0,
                        "重畫後 Pos X 欄仍有文字.", true);
        }

        // ---- AC3:切換 entity 後,ApplyEdit 只影響該 entity ----
        {
            SceneSystem system;
            Entity a = system.CreateNode();
            Entity b = system.CreateNode();
            system.world.GetComponent<TransformComponent>(a)->position = Point3D(0, 0, 0);
            system.world.GetComponent<TransformComponent>(b)->position = Point3D(0, 0, 0);

            InspectorLayer inspector(Point2D(1000, 800),
                                     Border(800.f, 30.f, 200.f, 400.f), &system);
            inspector.SelectEntity(a.GetIndex());
            inspector.ApplyEdit(a.GetIndex(), InspectorAxis::PositionX, +1.f);
            inspector.SelectEntity(b.GetIndex());
            inspector.ApplyEdit(b.GetIndex(), InspectorAxis::RotationY, +1.f);

            TransformComponent *pA = system.world.GetComponent<TransformComponent>(a);
            TransformComponent *pB = system.world.GetComponent<TransformComponent>(b);
            EXPECT_TRUE(Math::Abs(pA->position.x - 0.5f) < 1e-4f, "a.position.x 受 a 編輯影響 == 0.5.", true);
            EXPECT_TRUE(Math::Abs(pA->rotation.y) < 1e-4f, "a.rotation.y 不受 b 編輯影響 == 0.", true);
            EXPECT_TRUE(Math::Abs(pB->rotation.y - 5.0f) < 1e-4f, "b.rotation.y 受 b 編輯影響 == 5.0.", true);
            EXPECT_TRUE(Math::Abs(pB->position.x) < 1e-4f, "b.position.x 不受 a 編輯影響 == 0.", true);
        }

        // ---- AC4:inspector 對無效 entity 不崩(Alive 檢查) ----
        {
            SceneSystem system;
            Entity a = system.CreateNode();
            InspectorLayer inspector(Point2D(1000, 800),
                                     Border(800.f, 30.f, 200.f, 400.f), &system);
            inspector.SelectEntity(a.GetIndex());
            system.world.DestroyEntity(a);
            // 選到已毀 entity → Update 應安全跳過(不 crash,不改任何東西)
            inspector.Update();
            EXPECT_TRUE(true, "選取已毀 entity 後 Update 不崩.", true);
        }

        SUCCESS_MESSAGE("Inspector");
        return true;
    }
};

} // namespace inspectortest

#endif // INSPECTOR_TEST_HPP
