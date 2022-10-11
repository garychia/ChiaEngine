#include "Data/DataModule.hpp"
#include "Module.hpp"
#include "System/SystemModule.hpp"
#include "TestInfo.h"

#include <vector>

class AllModules
{
  private:
    std::vector<Module> modules;

  public:
    AllModules() : modules()
    {
        modules.push_back(DataModule());
        modules.push_back(SystemModule(String(IO_TEST_FILE_PATH)));
    }

    bool Run()
    {
        bool allSuccessful = true;
        for (auto &module : modules)
            allSuccessful = allSuccessful && module.Run();
        return allSuccessful;
    }
};
