#include "SystemModuleStandalone.hpp"

SystemModuleStandalone::SystemModuleStandalone() : Module()
{
    AddTest<SystemOperationTest>();
}