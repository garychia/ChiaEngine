#include "SystemModule.hpp"
#include "AssetTest.hpp"
#include "CameraControllerTest.hpp"
#include "IO/IOTest.hpp"
#include "ModuleTest.hpp"
#include "ReplayTest.hpp"
#include "WorldTest.hpp"

SystemModule::SystemModule(const String &ioTestPath)
{
    AddTest<assettest::AssetTest>();
    AddTest<IOTest>(ioTestPath);
    AddTest<SystemOperationTest>();
    AddTest<ModuleTest>();
    AddTest<WorldTest>();
    AddTest<replaytest::ReplayTest>();
    AddTest<CameraControllerTest>();
}