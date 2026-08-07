#include "SystemModuleStandalone.hpp"
#include "FrameCounterTest.hpp"
#include "PhysicsOverlapTest.hpp"
#include "PhysicsSystemTest.hpp"
#include "SceneSystemTest.hpp"
#include "AssetTest.hpp"
#include "TextureAssetViewTest.hpp"
#include "FrameTest.hpp"
#include "FrameSerializationTest.hpp"
#include "TextRenderingTest.hpp"

SystemModuleStandalone::SystemModuleStandalone() : Module()
{
    AddTest<SystemOperationTest>();
    AddTest<assettest::AssetTest>();
    AddTest<textureviewtest::TextureAssetViewTest>();
    AddTest<framecountertest::FrameCounterTest>();
    AddTest<overlap_test::PhysicsOverlapTest>();
    AddTest<physicssystemtest::PhysicsSystemTest>();
    AddTest<scenesystemtest::SceneSystemTest>();
    AddTest<FrameTest>();
    AddTest<FrameSerializationTest>();
    AddTest<TextRenderingTest>();
}