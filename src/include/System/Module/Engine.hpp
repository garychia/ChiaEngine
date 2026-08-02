#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "Data/DynamicArray.hpp"
#include "EngineContext.hpp"
#include "FrameClock.hpp"
#include "IModule.hpp"

// 組合根:擁有 FrameClock 與 EngineContext,驅動所有模組的生命週期與心跳。
//
// Tick(frameDeltaSeconds):
//   1. 依序跑 fixedStepsPerFrame 次 FixedUpdate(固定步進,確定性)
//   2. 跑一次 Update(每幀)
//   3. 推進 frameIndex
//
// 模組以附加順序執行 — 順序本身就是確定性的一部分。
// 注意:Engine 不擁有模組(儲存裸指標),所有權留在組合根(App)。
class Engine
{
  private:
    EngineContext context;
    DynamicArray<IModule *> modules;
    FrameClock clock;
    uint32_t fixedStepsPerFrame;

  public:
    Engine(uint32_t fixedStepsPerFrame = 1)
        : context(), modules(), clock(), fixedStepsPerFrame(fixedStepsPerFrame)
    {
    }

    EngineContext &GetContext()
    {
        return context;
    }

    const FrameClock &GetClock() const
    {
        return clock;
    }

    void Attach(IModule *pModule)
    {
        pModule->OnAttach(context);
        modules.Append(pModule);
    }

    void Detach(IModule *pModule)
    {
        for (size_t i = 0; i < modules.GetNElements(); i++)
        {
            if (modules[i] == pModule)
            {
                pModule->OnDetach(context);
                modules[i] = modules.GetLast();
                modules.RemoveLast();
                return;
            }
        }
    }

    void Tick(double frameDeltaSeconds)
    {
        clock.frameDeltaSeconds = frameDeltaSeconds;
        for (uint32_t step = 0; step < fixedStepsPerFrame; step++)
        {
            for (size_t i = 0; i < modules.GetNElements(); i++)
                modules[i]->FixedUpdate(clock);
            clock.tickIndex++;
        }
        for (size_t i = 0; i < modules.GetNElements(); i++)
            modules[i]->Update(clock);
        clock.frameIndex++;
    }
};

#endif // ENGINE_HPP
