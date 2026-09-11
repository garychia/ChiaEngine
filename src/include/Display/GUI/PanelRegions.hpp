#ifndef PANEL_REGIONS_HPP
#define PANEL_REGIONS_HPP

#include "Display/GUI/Border.hpp"
#include "Geometry/2D/Point2D.hpp"

// ADR-0001 D5-resize contract (headless-testable, no GUI/GLFW deps):
// all editor region constants + the pure region math live here so tests can
// verify the layout contract without a window. PanelLayout/Panel consume it.
namespace PanelLayoutConstants
{
inline constexpr long TopBarHeight = 30;
inline constexpr long SidebarWidth = 180;
inline constexpr long RowHeight = 22;
inline constexpr long InspectorWidth = 200;
} // namespace PanelLayoutConstants

// 依 window 尺寸 + scene aspect ratio 計算全部 editor regions。
struct PanelRegions
{
    Border topBar;          // (0, 0, W, TopBarHeight)
    Border leftDock;        // (0, TopBarHeight, SidebarWidth, H - TopBarHeight)
    Border rightDock;       // (W - InspectorWidth, TopBarHeight, InspectorWidth, H - TopBarHeight)
    Border centerViewport;  // ((W-vw)/2, TopBarHeight, vw, H - TopBarHeight) — SceneWindow child
    Point2D sceneSize;      // viewport w/h (from sceneWidthHeightRatio)
};

inline PanelRegions ComputePanelRegions(long windowWidth, long windowHeight,
                                        const Point2D &sceneWidthHeightRatio)
{
    using namespace PanelLayoutConstants;
    PanelRegions r;
    const float W = static_cast<float>(windowWidth);
    const float H = static_cast<float>(windowHeight);
    const float topH = static_cast<float>(TopBarHeight);
    const float sbW = static_cast<float>(SidebarWidth);
    const float inspW = static_cast<float>(InspectorWidth);

    r.topBar = Border(0.f, 0.f, W, topH);
    r.leftDock = Border(0.f, topH, sbW, H - topH);
    r.rightDock = Border(W - inspW, topH, inspW, H - topH);

    // viewport 佔中段,依 sceneWidthHeightRatio 推導(保持 aspect)。
    const float workH = H - topH;
    float vw = workH * sceneWidthHeightRatio.x / sceneWidthHeightRatio.y;
    float vh = workH;
    // 若左右 dock 佔太寬,退回剩餘寬度。
    const float availW = W - sbW - inspW;
    if (vw > availW)
    {
        vw = availW;
        vh = vw * sceneWidthHeightRatio.y / sceneWidthHeightRatio.x;
        if (vh > workH)
            vh = workH;
    }
    r.centerViewport = Border((W - vw) / 2.f, topH, vw, vh);
    r.sceneSize = Point2D(vw, vh);
    return r;
}

#endif // PANEL_REGIONS_HPP