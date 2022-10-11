#ifndef APP_HPP
#define APP_HPP

#include "Data/DynamicArray.hpp"
#include "Display/Window.hpp"
#include "Display/WindowInfo.hpp"
#include "pch.hpp"

class App
{
  private:
    Window *pMainWindow;
    Window *pSceneWindow;
    DynamicArray<Texture *> pTextures;
    SharedPtr<Camera> mainCamera;
    SharedPtr<Scene> mainScene;

    bool Initialize();

    void Finalize();

    void LoadData();

    void Update();

    void Render();

    bool PanelKeyInputHandler(const KeyCombination &combination);

    bool PanelMouseInputHandler(const MouseInfo &mouseInfo);

    bool SceneKeyInputHandler(const KeyCombination &combination);

    bool SceneMouseInputHandler(const MouseInfo &mouseInfo);

  public:
    App();

    ~App();

    int Execute();
};

#endif
