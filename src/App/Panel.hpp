#ifndef PANEL_HPP
#define PANEL_HPP

#include "Data/DynamicArray.hpp"
#include "Data/Pointers.hpp"
#include "Display/GUI/HorizontalList.hpp"
#include "SceneWindow.hpp"

class Panel : public Window
{
  private:
    static const unsigned long TopBarHeight;

    SharedPtr<HorizontalList> topBar;

    Scene GUIScene;

    SceneWindow *pSceneWindow;

    Point2D sceneWidthHeightRatio;

    Panel(const WindowInfo &info);

  public:
    ~Panel();

    virtual bool Initialize(Window *pParent = nullptr) override;

    virtual void Render() override;

    virtual void OnWindowResized(long newWidth, long newHeight) override;

    virtual bool OnKeyboardInputReceived(const KeyCombination &keys) override;

    virtual bool OnMouseInputReceived(const MouseInfo &mouseInfo) override;

    friend class WindowManager;
};

#endif // PANEL_HPP
