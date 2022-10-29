#ifndef CHIA_APP_HPP
#define CHIA_APP_HPP

#include "App/App.hpp"
#include "Panel.hpp"

class ChiaApp : public App
{
  public:
    ChiaApp(const AppInfo &info);

    virtual int Execute() override;
};

#endif
