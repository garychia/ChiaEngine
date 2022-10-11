#ifndef SYSTEM_MODULE_HPP
#define SYSTEM_MODULE_HPP

#include "Data/String.hpp"
#include "Module.hpp"


class SystemModule : public Module
{
  public:
    SystemModule(const String &ioTestPath);
};

#endif
