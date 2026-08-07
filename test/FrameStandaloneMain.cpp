// 無頭 standalone 執行器:只跑 Frame 命令流 + 序列化測試。
// 這是 P7d 的無 GPU CI 驗收目標(驗收 1、2)——在 CMakeLists_standalone.txt 的
// FrameStandaloneTest target 編譯/執行,不依賴任何圖形後端 SDK。
#include "Module.hpp"
#include "Test.hpp"
#include "System/FrameTest.hpp"
#include "System/FrameSerializationTest.hpp"
#include "System/TextRenderingTest.hpp"

class FrameStandaloneModule : public Module
{
  public:
    FrameStandaloneModule() : Module()
    {
        AddTest<FrameTest>();
        AddTest<FrameSerializationTest>();
        AddTest<TextRenderingTest>();
    }
};

int main()
{
    FrameStandaloneModule moduleTest;
    const bool allOk = moduleTest.Run();
    return allOk ? 0 : 1;
}