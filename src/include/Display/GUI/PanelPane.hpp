#ifndef PANEL_PANE_HPP
#define PANEL_PANE_HPP

#include "Data/String.hpp"
#include "Display/GUI/GUILayer.hpp"

// ADR-0001 D3:layout 的單位 — named pane。
// 取代「直接在 PanelLayout 構造層 + SetRegions 特判(GetLayers()[0] 魔術 index、
// 成員 slot)」的老路:一個新面板 = 註冊一個 named pane(dock + title + collapse),
// 由 dock manager 依 dock 自動定位。

// ADR-0001 D3:named dock 區(與 PanelRegions::PanelRegions 的 dock 一對一)。
enum class PanelDock
{
    TopBar,         // 頂部工具列(toolbar)
    LeftDock,       // 左側(hierarchy 側欄)
    RightDock,      // 右側(inspector)
    CenterViewport, // 中間 3D viewport(SceneWindow child — GUI 層不使用)
};

// PanelPane : GUILayer — GUI 表面的單位:
// - title:pane 識別名稱(registry key,GetPane 用)
// - dock:所屬 dock 區(SetRegions 依此定位)
// - collapsed:收合狀態(SetRegions 把高度歸零,隱藏內容)
class PanelPane : public GUILayer
{
  public:
    PanelPane(const String &title, const Point2D &windowSize, const Border &border,
              PanelDock dock = PanelDock::LeftDock)
        : GUILayer(windowSize, border), mTitle(title), mDock(dock), mCollapsed(false)
    {
    }

    const String &GetTitle() const
    {
        return mTitle;
    }

    PanelDock GetDock() const
    {
        return mDock;
    }

    void SetDock(PanelDock dock)
    {
        mDock = dock;
    }

    bool IsCollapsed() const
    {
        return mCollapsed;
    }

    void SetCollapsed(bool collapsed)
    {
        mCollapsed = collapsed;
    }

  private:
    String mTitle;
    PanelDock mDock;
    bool mCollapsed;
};

#endif // PANEL_PANE_HPP