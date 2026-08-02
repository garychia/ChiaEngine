#include "SystemModule.hpp"
#include "IO/IOTest.hpp"
#include "ModuleTest.hpp"

SystemModule::SystemModule(const String &ioTestPath)
{
    AddTest<IOTest>(ioTestPath);
    AddTest<SystemOperationTest>();
    AddTest<ModuleTest>();
}