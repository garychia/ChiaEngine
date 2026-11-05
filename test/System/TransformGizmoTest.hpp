#ifndef TRANSFORM_GIZMO_TEST_HPP
#define TRANSFORM_GIZMO_TEST_HPP

#include "Display/GUI/TransformGizmo.hpp"
#include "Geometry/3D/Point3D.hpp"
#include "Test.hpp"

#include <cmath>

namespace transformgizmo_test
{

// TransformGizmoTest:A4.1 Transform Gizmo 純數學契約。
// 1. 軸選取:從射線命中 X/Y/Z 軸,無選中回 Axis::None
// 2. 平移增量:沿選中軸計算位移向量
// 3. 旋轉增量:繞選中軸計算角度(度)
// 4. 縮放增量:沿選中軸計算縮放係數
// 5. 軸顏色與線段端點
class TransformGizmoTest : public Test
{
  public:
    TransformGizmoTest() : Test("TransformGizmo")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:軸選取 ----
        {
            TransformGizmo gizmo;
            gizmo.SetTargetEntity(1);
            gizmo.SetWorldPosition(Point3D(0.f, 0.f, 0.f));
            gizmo.SetAxisLength(10.f);
            gizmo.SetPickThreshold(0.5f);

            // 無選中實體 → Axis::None
            gizmo.SetTargetEntity(0);
            EXPECT_TRUE(gizmo.GetSelectedAxis() == TransformGizmo::Axis::None,
                        "無目標 → Axis::None", true);

            gizmo.SetTargetEntity(1);

            // 射線沿 X 軸方向,原點在 X 軸上偏移 0.1 → 應選 X 軸
            const auto axisX = gizmo.GetAxisFromRay(Point3D(5.f, 0.1f, 0.f), Point3D(0.f, -1.f, 0.f));
            EXPECT_TRUE(axisX == TransformGizmo::Axis::X,
                        "射線命中 X 軸 → Axis::X", true);

            // 射線遠離所有軸(指向外方,不穿過原點) → Axis::None
            const auto axisNone = gizmo.GetAxisFromRay(Point3D(100.f, 100.f, 100.f),
                                                       Point3D(1.f, 1.f, 1.f));
            EXPECT_TRUE(axisNone == TransformGizmo::Axis::None,
                        "射線遠離所有軸 → Axis::None", true);
        }

        // ---- AC2:平移增量 ----
        {
            TransformGizmo gizmo;
            gizmo.SetTargetEntity(1);
            gizmo.SetWorldPosition(Point3D(0.f, 0.f, 0.f));
            gizmo.SetAxisLength(10.f);
            gizmo.SetPickThreshold(1.0f);
            gizmo.SetMode(TransformGizmo::Mode::Translate);

            // 無選中軸 → 零向量
            const auto dz = gizmo.GetTranslateDelta(Point3D(0, 0, 0), Point3D(1, 0, 0));
            EXPECT_TRUE(dz.x == 0.f && dz.y == 0.f && dz.z == 0.f,
                        "無軸選中 → 零向量", true);

            gizmo.GetAxisFromRay(Point3D(5.f, 0.1f, 0.f), Point3D(0.f, -1.f, 0.f)); // 選 X 軸

            // 沿 X 軸拖曳 (1,0,0) → (4,0,0):增量 (3,0,0)
            const auto dx = gizmo.GetTranslateDelta(Point3D(1.f, 0.f, 0.f), Point3D(4.f, 0.f, 0.f));
            EXPECT_TRUE(dx.x == 3.f && dx.y == 0.f && dx.z == 0.f,
                        "X 軸平移 → (3,0,0)", true);

            // 垂直於 X 軸的拖曳不應產生 X 分量
            const auto dy = gizmo.GetTranslateDelta(Point3D(0.f, 0.f, 0.f), Point3D(0.f, 5.f, 0.f));
            EXPECT_TRUE(dy.x == 0.f && dy.y == 0.f && dy.z == 0.f,
                        "Y 方向拖曳在 X 軸模式 → 零增量", true);
        }

        // ---- AC3:旋轉增量 ----
        {
            TransformGizmo gizmo;
            gizmo.SetTargetEntity(1);
            gizmo.SetWorldPosition(Point3D(0.f, 0.f, 0.f));
            gizmo.SetAxisLength(10.f);
            gizmo.SetPickThreshold(1.0f);
            gizmo.SetMode(TransformGizmo::Mode::Rotate);

            // 無選中軸 → 0 度
            EXPECT_TRUE(gizmo.GetRotateDelta(Point3D(0, 0, 0), Point3D(1, 0, 0)) == 0.f,
                        "無軸選中 → 0 度", true);

            gizmo.GetAxisFromRay(Point3D(5.f, 0.1f, 0.f), Point3D(0.f, -1.f, 0.f)); // 選 X 軸

            // 在 YZ 平面繞 X 軸旋轉 90 度:從 (0,10,0) 到 (0,0,10)
            const auto rot = gizmo.GetRotateDelta(Point3D(0.f, 10.f, 0.f), Point3D(0.f, 0.f, 10.f));
            EXPECT_TRUE(std::abs(rot - 90.f) < 1.f,
                        "繞 X 軸旋轉 90 度 → ~90 度", true);
        }

        // ---- AC4:縮放增量 ----
        {
            TransformGizmo gizmo;
            gizmo.SetTargetEntity(1);
            gizmo.SetWorldPosition(Point3D(0.f, 0.f, 0.f));
            gizmo.SetAxisLength(10.f);
            gizmo.SetPickThreshold(1.0f);
            gizmo.SetMode(TransformGizmo::Mode::Scale);

            // 無選中軸 → 1.0
            EXPECT_TRUE(gizmo.GetScaleDelta(Point3D(0, 0, 0), Point3D(1, 0, 0)) == 1.f,
                        "無軸選中 → 1.0", true);

            gizmo.GetAxisFromRay(Point3D(5.f, 0.1f, 0.f), Point3D(0.f, -1.f, 0.f)); // 選 X 軸

            // 沿 X 軸正向拖曳 1 單位 → 係數 1.0 + 1*10 = 11.0
            // 但 axisLength=10, t=1 → 1+1*10=11
            const auto sx = gizmo.GetScaleDelta(Point3D(0.f, 0.f, 0.f), Point3D(1.f, 0.f, 0.f));
            EXPECT_TRUE(std::abs(sx - 11.f) < 0.01f,
                        "X 軸正向拖曳 1 單位 → 係數 11.0", true);

            // 沿 X 軸負向拖曳 → 係數 < 1
            const auto sxNeg = gizmo.GetScaleDelta(Point3D(0.f, 0.f, 0.f), Point3D(-1.f, 0.f, 0.f));
            EXPECT_TRUE(sxNeg < 1.f && sxNeg > 0.f,
                        "X 軸負向拖曳 → 係數在 (0,1)", true);
        }

        // ---- AC5:軸顏色與線段端點 ----
        {
            TransformGizmo gizmo;
            gizmo.SetWorldPosition(Point3D(1.f, 2.f, 3.f));
            gizmo.SetAxisLength(5.f);

            // X 軸顏色 = 紅
            const auto cx = gizmo.GetAxisColor(TransformGizmo::Axis::X);
            EXPECT_TRUE(cx.x == 1.f && cx.y == 0.f && cx.z == 0.f,
                        "X 軸顏色 = 紅", true);

            // Y 軸顏色 = 綠
            const auto cy = gizmo.GetAxisColor(TransformGizmo::Axis::Y);
            EXPECT_TRUE(cy.x == 0.f && cy.y == 1.f && cy.z == 0.f,
                        "Y 軸顏色 = 綠", true);

            // Z 軸顏色 = 藍
            const auto cz = gizmo.GetAxisColor(TransformGizmo::Axis::Z);
            EXPECT_TRUE(cz.x == 0.f && cz.y == 0.f && cz.z == 1.f,
                        "Z 軸顏色 = 藍", true);

            // X 軸線段: (1,2,3) → (6,2,3)
            const auto lx = gizmo.GetAxisLine(TransformGizmo::Axis::X);
            EXPECT_TRUE(lx.start.x == 1.f && lx.start.y == 2.f && lx.start.z == 3.f &&
                            lx.end.x == 6.f && lx.end.y == 2.f && lx.end.z == 3.f,
                        "X 軸線段端點正確", true);

            // 無軸線段: start == end
            const auto ln = gizmo.GetAxisLine(TransformGizmo::Axis::None);
            EXPECT_TRUE(ln.start.x == ln.end.x && ln.start.y == ln.end.y && ln.start.z == ln.end.z,
                        "無軸線段 start==end", true);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }

  private:
};

} // namespace transformgizmo_test

#endif // TRANSFORM_GIZMO_TEST_HPP
