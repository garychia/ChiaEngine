#include "SystemModuleStandalone.hpp"
#include "FrameCounterTest.hpp"
#include "PhysicsOverlapTest.hpp"
#include "PhysicsSystemTest.hpp"
#include "SceneSystemTest.hpp"
#include "AssetTest.hpp"
#include "TextureAssetViewTest.hpp"
#include "FrameTest.hpp"
#include "FrameSerializationTest.hpp"
#include "RendererContractTest.hpp"
#include "FrameInterpreterTest.hpp"
#include "TextRenderingTest.hpp"
#include "PongTest.hpp" // #89: Pong demo 無頭測試(Sim 層隔離)

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
    AddTest<renderercontracttest::RendererContractTest>();
    AddTest<frameinterpretertest::FrameInterpreterTest>(); // #84:真實 seam(純解譯器)
    AddTest<renderercontracttest::VulkanRendererTest>();
    AddTest<pongtest::PongTest>(); // #89: Pong demo 無頭測試
}