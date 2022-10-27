#ifndef SCENE_WINDOW_HPP
#define SCENE_WINDOW_HPP

#include "Display/Window.hpp"

class SceneWindow : public Window
{
  private:
    DynamicArray<SharedPtr<Texture>> pTextures;

    SharedPtr<Camera> pMainCamera;

    SharedPtr<Scene> pMainScene;

    SceneWindow(const WindowInfo &info);

  public:
    ~SceneWindow();

    virtual bool Initialize(Window *pParent = nullptr) override;

    virtual bool OnKeyboardInputReceived(const KeyCombination &combination) override;

    virtual bool OnMouseInputReceived(const MouseInfo &mouseInfo) override;

    friend class WindowManager;
};

#endif // SCENE_WINDOW_HPP
