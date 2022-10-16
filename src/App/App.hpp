#ifndef APP_HPP
#define APP_HPP

#include "App/Panel.hpp"
#include "Data/DynamicArray.hpp"
#include "Display/WindowInfo.hpp"
#include "pch.hpp"

class App
{
  private:
    Panel *pMainWindow;

    bool Initialize();

    void Finalize();

    bool LoadData();

    void Update();

    void Render();

  public:
    App();

    ~App();

    int Execute();
};

#endif
