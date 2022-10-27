#ifndef CHIA_APP_HPP
#define CHIA_APP_HPP

#include "App/App.hpp"
#include "Panel.hpp"

class ChiaApp : public App
{
  private:
    Panel *pMainWindow;

    bool fullScreen;

  protected:
    virtual bool Initialize() override;

    virtual void Finalize() override;

    virtual bool LoadData() override;

  public:
    ChiaApp(const AppInfo &info);

    virtual int Execute() override;
};

#endif
