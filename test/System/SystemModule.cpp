#include "SystemModule.hpp"
#include "AssetTest.hpp"
#include "CameraControllerTest.hpp"
#include "FrameCounterTest.hpp"
#include "IO/IOTest.hpp"
#include "ModuleTest.hpp"
#include "PhysicsOverlapTest.hpp"
#include "PhysicsSystemTest.hpp"
#include "SceneSystemTest.hpp"
#include "ReplayTest.hpp"
#include "WorldTest.hpp"

SystemModule::SystemModule(const String &ioTestPath)
{
    AddTest<assettest::AssetTest>();
    AddTest<IOTest>(ioTestPath);
    AddTest<SystemOperationTest>();
    AddTest<ModuleTest>();
    AddTest<WorldTest>();
    AddTest<framecountertest::FrameCounterTest>();
    AddTest<replaytest::ReplayTest>();
    AddTest<CameraControllerTest>();
    AddTest<overlap_test::PhysicsOverlapTest>();
    AddTest<physicssystemtest::PhysicsSystemTest>();
    AddTest<scenesystemtest::SceneSystemTest>();
}