#ifndef SYSTEM_FRAMECOUNTER_TEST_HPP
#define SYSTEM_FRAMECOUNTER_TEST_HPP

#include "Test.hpp"
#include "System/Module/Engine.hpp"
#include "System/Module/FrameCounter.hpp"
#include "System/Module/IModule.hpp"

// 用 namespace 隔離,避免與 SystemModule.cpp 內其它測試的全域型別衝突
// (同 ReplayTest.hpp 的做法)。
namespace framecountertest
{

// 訂閱探針:OnAttach 時從 context 解析 FrameCounter 並訂閱 onFrameProduced,
// 逐 tick 記錄收到的索引,以供測試驗證完整序列。
class FrameListener : public IModule
{
  public:
    uint32_t count = 0;
    uint32_t lastSeen = 0;
    bool monotonic = true;

    void OnAttach(EngineContext &context) override
    {
        FrameCounter *pCounter = context.ResolveService<FrameCounter>();
        pCounter->onFrameProduced.Subscribe(this, &FrameListener::OnFrameProduced);
    }

    void OnDetach(EngineContext &context) override
    {
        FrameCounter *pCounter = context.ResolveService<FrameCounter>();
        pCounter->onFrameProduced.Unsubscribe(this);
    }

    void OnFrameProduced(OnFrameProduced event)
    {
        if (count > 0 && event.frameIndex != lastSeen + 1)
            monotonic = false;
        lastSeen = event.frameIndex;
        count++;
    }

    void Update(const FrameClock &clock) override
    {
        (void)clock;
    }
};

// ---------------- 測試主體 ----------------

class FrameCounterTest : public Test
{
  public:
    FrameCounterTest() : Test("SystemFrameCounter")
    {
    }

    bool Run() noexcept override
    {
        const int N = 200;

        TEST_MESSAGE("FrameCounter emits 0 then +1 per tick; subscriber sees sequence");
        {
            Engine engine(1);
            FrameCounter counter;
            FrameListener listener;
            // 附著順序契約:FrameCounter 先附著,訂閱者才能在 OnAttach 解析到服務
            engine.Attach(&counter);
            engine.Attach(&listener);

            for (int i = 0; i < N; i++)
                engine.Tick(1.0 / 60.0);

            // AC1:每 tick 恰好 +1,從 0 開始 → 跑了 N tick 後計數器 == N
            EXPECT_TRUE(counter.GetFrameIndex() == static_cast<uint32_t>(N), "Frame index = N after N ticks.", true);
            // AC2:訂閱者透過 bus 收到完整的 0..N-1 序列
            EXPECT_TRUE(listener.count == static_cast<uint32_t>(N), "Listener received one event per tick.", true);
            EXPECT_TRUE(listener.lastSeen == static_cast<uint32_t>(N - 1), "Last emitted index was N-1.", true);
            EXPECT_TRUE(listener.monotonic, "Received indices increment by exactly 1, starting at 0.", true);
        }

        TEST_MESSAGE("Detach unsubscribes; fresh counter restarts at 0");
        {
            Engine engine(1);
            FrameCounter counter;
            FrameListener listener;
            engine.Attach(&counter);
            engine.Attach(&listener);
            engine.Tick(1.0 / 60.0);
            EXPECT_TRUE(listener.count == 1 && listener.lastSeen == 0, "First tick emits 0.", true);
            engine.Detach(&listener);
            engine.Tick(1.0 / 60.0);
            EXPECT_TRUE(listener.count == 1 && listener.lastSeen == 0, "Detached listener receives nothing more.", true);
            EXPECT_TRUE(counter.GetFrameIndex() == 2, "Counter keeps incrementing independent of subscribers.", true);
        }

        SUCCESS_MESSAGE("SystemFrameCounter");
        return true;
    }
};

} // namespace framecountertest

#endif // SYSTEM_FRAMECOUNTER_TEST_HPP