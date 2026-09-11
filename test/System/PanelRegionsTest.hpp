#ifndef PANEL_REGIONS_TEST_HPP
#define PANEL_REGIONS_TEST_HPP

#include "Display/GUI/PanelRegions.hpp"
#include "Math/Math.hpp"
#include "Test.hpp"

// #74 ADR-0001 D5-resize:PanelRegions + ComputePanelRegions contract.
// 純數學(headless):驗證 toolbar/left/right dock + viewport(含 aspect 縮放)
// 從 window 尺寸的計算。PanelLayout::SetRegions 只是把結果套到 layer。
namespace panelregionstest
{

class PanelRegionsTest : public Test
{
  public:
    PanelRegionsTest() : Test("PanelRegions")
    {
    }

    bool Run() noexcept override
    {
        // ---- AC1:compact 視窗(1000x800, ratio 4:3)→ viewport clamp 到剩餘寬度 ----
        {
            const PanelRegions r = ComputePanelRegions(1000, 800, Point2D(4, 3));
            // top bar: 全寬,高 30
            EXPECT_TRUE(Math::Abs((float)r.topBar.width - 1000.f) < 1.f, "topBar 寬 = W.", true);
            EXPECT_TRUE(Math::Abs((float)r.topBar.height - 30.f) < 1.f, "topBar 高 = TopBarHeight(30).", true);
            // left dock
            EXPECT_TRUE(Math::Abs((float)r.leftDock.width - 180.f) < 1.f, "leftDock 寬 = SidebarWidth(180).", true);
            EXPECT_TRUE(Math::Abs((float)r.leftDock.height - 770.f) < 1.f, "leftDock 高 = H - TopBarHeight.", true);
            // right dock
            EXPECT_TRUE(Math::Abs((float)r.rightDock.xPos - 800.f) < 1.f, "rightDock x = W - InspectorWidth.", true);
            EXPECT_TRUE(Math::Abs((float)r.rightDock.width - 200.f) < 1.f, "rightDock 寬 = InspectorWidth(200).", true);
            // viewport:vw=770*4/3=1026.7 > availW(620)→ clamp 620,vh=620*3/4=465
            EXPECT_TRUE(Math::Abs((float)r.sceneSize.x - 620.f) < 1.f, "viewport 寬 clamp 到剩餘寬度 620.", true);
            EXPECT_TRUE(Math::Abs((float)r.sceneSize.y - 465.f) < 1.f, "viewport 高依 4:3 ratio = 465.", true);
            EXPECT_TRUE(Math::Abs((float)r.centerViewport.xPos - 190.f) < 1.f, "viewport 水平居中 ((1000-620)/2=190).", true);
            EXPECT_TRUE(Math::Abs((float)r.centerViewport.yPos - 30.f) < 1.f, "viewport y = TopBarHeight.", true);
        }

        // ---- AC2:寬視窗(2000x800)→ viewport 不 clamp,vh = workH ----
        {
            const PanelRegions r = ComputePanelRegions(2000, 800, Point2D(4, 3));
            // vw = 770 * 4/3 = 1026.7 < availW(1620) → 不 clamp
            EXPECT_TRUE(Math::Abs((float)r.sceneSize.x - 1026.67f) < 1.f, "寬窗 viewport 寬 = workH*ratio.", true);
            EXPECT_TRUE(Math::Abs((float)r.sceneSize.y - 770.f) < 1.f, "寬窗 viewport 高 = workH.", true);
            EXPECT_TRUE(Math::Abs((float)r.centerViewport.xPos - ((2000.f - 1026.67f) / 2.f)) < 1.f,
                        "寬窗 viewport 水平居中.", true);
        }

        // ---- AC3:極窄視窗(600x800)→ sceneSize 不為負,右 dock 不越界 ----
        {
            const PanelRegions r = ComputePanelRegions(600, 800, Point2D(4, 3));
            // availW = 600 - 180 - 200 = 220;vw=1026 > 220 → clamp 220,vh=165
            EXPECT_TRUE(Math::Abs((float)r.sceneSize.x - 220.f) < 1.f, "窄窗 viewport 寬 clamp 到 220.", true);
            EXPECT_TRUE(Math::Abs((float)r.sceneSize.y - 165.f) < 1.f, "窄窗 viewport 高 = 220*3/4 = 165.", true);
            EXPECT_TRUE((float)r.sceneSize.x > 0.f && (float)r.sceneSize.y > 0.f, "窄窗 sceneSize 為正.", true);
            EXPECT_TRUE((float)r.rightDock.xPos + (float)r.rightDock.width <= 600.5f, "右 dock 不越出窗寬.", true);
        }

        SUCCESS_MESSAGE("PanelRegions: ComputePanelRegions contract 全部通過.");
        return true;
    }
};

} // namespace panelregionstest

#endif // PANEL_REGIONS_TEST_HPP