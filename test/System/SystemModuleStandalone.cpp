#include "SystemModuleStandalone.hpp"
#include "FrameCounterTest.hpp"

SystemModuleStandalone::SystemModuleStandalone() : Module()
{
    AddTest<SystemOperationTest>();
    AddTest<framecountertest::FrameCounterTest>();
}