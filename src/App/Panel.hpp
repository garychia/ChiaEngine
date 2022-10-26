#ifndef PANEL_HPP
#define PANEL_HPP

#include "Data/DynamicArray.hpp"
#include "Data/Pointers.hpp"
#include "SceneWindow.hpp"
#include "Components/PanelLayout.hpp"

class Panel : public Window
{
  private:
    SceneWindow *pSceneWindow;

    Point2D sceneWidthHeightRatio;

    PanelLayout layout;

    Panel(const WindowInfo &info);

  public:
    virtual bool Initialize(Window *pParent = nullptr) override;

    virtual void Render() override;

    virtual void OnWindowResized(long newWidth, long newHeight) override;

    virtual bool OnKeyboardInputReceived(const KeyCombination &keys) override;

    virtual bool OnMouseInputReceived(const MouseInfo &mouseInfo) override;

    friend class WindowManager;
};

#endif // PANEL_HPP
