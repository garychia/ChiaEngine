#ifndef APP_HPP
#define APP_HPP

#include "Data/String.hpp"
#include "Display/Window.hpp"
#include "MainLoop.hpp"
#include "Version.hpp"

struct AppInfo
{
    String appName;

    Version appVersion;

    Version engineVersion;
};

class App
{
  protected:
    AppInfo info;

    MainLoop mainLoop;

    Window *pMainWindow;

  public:
    App(const AppInfo &info);

    virtual ~App();

    virtual bool Initialize();

    virtual void Finalize();

    virtual int Execute();

    virtual void Update();

    virtual void Render();
};

#endif
