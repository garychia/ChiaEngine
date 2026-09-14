#include "SystemModule.hpp"
#include "AssetFormatValidationTest.hpp"
#include "AssetTest.hpp"
#include "CameraControllerTest.hpp"
#include "FrameCounterTest.hpp"
#include "GUITest.hpp"
#include "InspectorTest.hpp"
#include "EditorSessionTest.hpp"
#include "PanelRegionsTest.hpp"
#include "DockRegistryTest.hpp" // #87 ADR-0001 D3:named docks + PanelPane registry
#include "IO/IOTest.hpp"
#include "InputSystemTest.hpp"
#include "ModuleTest.hpp"
#include "PhysicsOverlapTest.hpp"
#include "PhysicsSystemTest.hpp"
#include "SceneSystemTest.hpp"
#include "ReplayTest.hpp"
#include "RendererContractTest.hpp"
#include "FrameInterpreterTest.hpp" // #84:真實 seam(純解譯器)契約測試
#include "TextureAssetViewTest.hpp"
#include "WorldTest.hpp"
#include "AudioSystemTest.hpp"
#include "PongTest.hpp" // #89: Pong demo 無頭測試(Sim 層隔離,無 GPU)

SystemModule::SystemModule(const String &ioTestPath)
{
    AddTest<assettest::AssetTest>();
    AddTest<assetfmt::MalformedCorpusTest>();
    AddTest<assetfmt::AssetRoundtripTest>();
    AddTest<assetfmt::TextureViewHookTest>();
    AddTest<textureviewtest::TextureAssetViewTest>();
    AddTest<IOTest>(ioTestPath);
    AddTest<operationtest::SystemOperationTest>();
    AddTest<ModuleTest>();
    AddTest<WorldTest>();
    AddTest<framecountertest::FrameCounterTest>();
    AddTest<guitest::GUITest>();
    AddTest<guitest::HierarchyRowTest>();
    AddTest<inspectortest::InspectorTest>();
    AddTest<editorsessiontest::EditorSessionTest>();
    AddTest<panelregionstest::PanelRegionsTest>();
    AddTest<dockregistrytest::DockRegistryTest>(); // #87 ADR-0001 D3:named docks + pane registry
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
    AddTest<frameinterpretertest::FrameInterpreterTest>(); // #84:真實 seam(純解譯器)
    AddTest<renderercontracttest::VulkanRendererTest>();
    AddTest<audiotest::AudioSystemTest>();
    AddTest<audiotest::AudioPlaybackTest>();
    AddTest<pongtest::PongTest>(); // #89: Pong demo 無頭測試
}