#include "SystemModule.hpp"
#include "AssetFormatValidationTest.hpp"
#include "AssetTest.hpp"
#include "CameraControllerTest.hpp"
#include "FrameCounterTest.hpp"
#include "GUITest.hpp"
#include "InspectorTest.hpp"
#include "IO/IOTest.hpp"
#include "InputSystemTest.hpp"
#include "ModuleTest.hpp"
#include "PhysicsOverlapTest.hpp"
#include "PhysicsSystemTest.hpp"
#include "SceneSystemTest.hpp"
#include "ReplayTest.hpp"
#include "RendererContractTest.hpp"
#include "TextureAssetViewTest.hpp"
#include "WorldTest.hpp"
#include "AudioSystemTest.hpp"

SystemModule::SystemModule(const String &ioTestPath)
{
    AddTest<assettest::AssetTest>();
    AddTest<assetfmt::MalformedCorpusTest>();
    AddTest<assetfmt::AssetRoundtripTest>();
    AddTest<assetfmt::TextureViewHookTest>();
    AddTest<textureviewtest::TextureAssetViewTest>();
    AddTest<IOTest>(ioTestPath);
    AddTest<SystemOperationTest>();
    AddTest<ModuleTest>();
    AddTest<WorldTest>();
    AddTest<framecountertest::FrameCounterTest>();
    AddTest<guitest::GUITest>();
    AddTest<guitest::HierarchyRowTest>();
    AddTest<inspectortest::InspectorTest>();
    AddTest<replaytest::ReplayTest>();
    AddTest<CameraControllerTest>();
    AddTest<inputtest::KeyCombinationTest>();
    AddTest<inputtest::KeyboardHandlerTest>();
    AddTest<inputtest::MouseInputTest>();
    AddTest<inputtest::InputHandlerTest>();
    AddTest<inputtest::WindowManagerTest>();
    AddTest<overlap_test::PhysicsOverlapTest>();
    AddTest<physicssystemtest::PhysicsSystemTest>();
    AddTest<scenesystemtest::SceneSystemTest>();
    AddTest<renderercontracttest::RendererContractTest>();
    AddTest<renderercontracttest::VulkanRendererTest>();
    AddTest<audiotest::AudioSystemTest>();
    AddTest<audiotest::AudioPlaybackTest>();
}