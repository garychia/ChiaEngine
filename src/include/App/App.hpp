#ifndef APP_HPP
#define APP_HPP

#include "Data/String.hpp"

struct AppInfo
{
    String appName;
};

class App
{
  protected:
    AppInfo info;

    virtual bool Initialize() = 0;

    virtual void Finalize() = 0;

    virtual bool LoadData() = 0;

    virtual void Update();

    virtual void Render();

  public:
    App(const AppInfo &info);

    virtual ~App();

    virtual int Execute();
};

#endif
