#include "Test.hpp"
#include "Module.hpp"

#include "System/Module/Engine.hpp"
#include "System/Module/IModule.hpp"
#include "System/Module/SimInput.hpp"
#include "System/Module/SimRecorder.hpp"
#include "System/World/Entity.hpp"
#include "System/World/World.hpp"
#include "System/Pong/PongSystem.hpp"
#include "System/Pong/PongHud.hpp"
#include "PongTest.hpp"

 // Pong 無頭 standalone 執行器:編譯獨立 pong 模組 + 測試,不碰 GPU/GLFW/Vulkan。
class PongModule : public Module
{
  public:
    PongModule() : Module()
    {
        AddTest<pongtest::PongTest>();
    }
};

int main()
{
    PongModule moduleTest;
    const bool allOk = moduleTest.Run();
    return allOk ? 0 : 1;
}