#include "SystemModule.hpp"
#include "IO/IOTest.hpp"

SystemModule::SystemModule(const String &ioTestPath)
{
    AddTest<IOTest>(ioTestPath);
}
