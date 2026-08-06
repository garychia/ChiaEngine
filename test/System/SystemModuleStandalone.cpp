#include "SystemModuleStandalone.hpp"
#include "FrameCounterTest.hpp"
#include "PhysicsOverlapTest.hpp"
#include "PhysicsSystemTest.hpp"
#include "SceneSystemTest.hpp"
#include "AssetTest.hpp"
#include "TextureAssetViewTest.hpp"

SystemModuleStandalone::SystemModuleStandalone() : Module()
{
    AddTest<SystemOperationTest>();
    AddTest<assettest::AssetTest>();
    AddTest<textureviewtest::TextureAssetViewTest>();
    AddTest<framecountertest::FrameCounterTest>();
    AddTest<overlap_test::PhysicsOverlapTest>();
    AddTest<physicssystemtest::PhysicsSystemTest>();
    AddTest<scenesystemtest::SceneSystemTest>();
}