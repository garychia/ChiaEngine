#ifndef SYSTEM_MODULE_TEST_HPP
#define SYSTEM_MODULE_TEST_HPP

#include "Test.hpp"
#include "System/Module/Engine.hpp"
#include "System/Module/EngineContext.hpp"
#include "System/Module/FrameClock.hpp"
#include "System/Module/IModule.hpp"
#include "System/Operation/Event.hpp"

// ---------------- 測試用模組 ----------------

class LifecycleProbe : public IModule
{
  public:
    int attached = 0;
    int detached = 0;
    int fixedTicks = 0;
    int updates = 0;

    void OnAttach(EngineContext &context) override
    {
        (void)context;
        attached++;
    }

    void OnDetach(EngineContext &context) override
    {
        (void)context;
        detached++;
    }

    void FixedUpdate(const FrameClock &clock) override
    {
        (void)clock;
        fixedTicks++;
    }

    void Update(const FrameClock &clock) override
    {
        (void)clock;
        updates++;
    }
};

class RenderService
{
  public:
    int draws = 0;

    void Draw()
    {
        draws++;
    }
};

class PhysicsService
{
  public:
    int steps = 0;

    void Step()
    {
        steps++;
    }
};

// 事件發送端:透過 EngineContext 註冊成服務,讓別人訂閱它的 Event
class EventEmitter : public IModule
{
  public:
    Event<void(int)> onPing;
};

// 事件接收端:OnAttach 時從 context 解析發送端並訂閱(Bus 模式)
class EventReceiver : public IModule
{
  public:
    int received = 0;

    void OnAttach(EngineContext &context) override
    {
        EventEmitter *pEmitter = context.ResolveService<EventEmitter>();
        pEmitter->onPing.Subscribe(this, &EventReceiver::OnPing);
    }

    void OnDetach(EngineContext &context) override
    {
        EventEmitter *pEmitter = context.ResolveService<EventEmitter>();
        pEmitter->onPing.Unsubscribe(this);
    }

    void OnPing(int value)
    {
        received += value;
    }
};

// ---------------- 測試主體 ----------------

class ModuleTest : public Test
{
  public:
    ModuleTest() : Test("SystemModule")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("Engine module lifecycle");
        {
            Engine engine;
            LifecycleProbe probe;
            engine.Attach(&probe);
            EXPECT_TRUE(probe.attached == 1, "OnAttach 被呼叫一次.", true);
            engine.Detach(&probe);
            EXPECT_TRUE(probe.detached == 1, "OnDetach 被呼叫一次.", true);
        }

        TEST_MESSAGE("Engine fixed-step tick");
        {
            Engine engine(3); // 每幀 3 個固定步進
            LifecycleProbe probe;
            engine.Attach(&probe);
            engine.Tick(1.0 / 60.0);
            EXPECT_TRUE(probe.fixedTicks == 3, "FixedUpdate 每 Tick 跑 3 次.", true);
            EXPECT_TRUE(probe.updates == 1, "Update 每 Tick 跑 1 次.", true);
            EXPECT_TRUE(engine.GetClock().tickIndex == 3, "tickIndex 推進.", true);
            EXPECT_TRUE(engine.GetClock().frameIndex == 1, "frameIndex 推進.", true);
            engine.Tick(1.0 / 60.0);
            EXPECT_TRUE(probe.fixedTicks == 6, "FixedUpdate 累加.", true);
            EXPECT_TRUE(probe.updates == 2, "Update 累加.", true);
        }

        TEST_MESSAGE("EngineContext service register/resolve");
        {
            EngineContext context;
            RenderService render;
            context.RegisterService<RenderService>(&render);
            RenderService *pResolved = context.ResolveService<RenderService>();
            EXPECT_TRUE(pResolved != nullptr, "服務可以解析.", true);
            pResolved->Draw();
            EXPECT_TRUE(render.draws == 1, "解析到的是註冊的那個實體.", true);
            EXPECT_TRUE(context.ResolveService<PhysicsService>() == nullptr, "未註冊型別解析為 nullptr.", true);
            context.UnregisterService<RenderService>();
            EXPECT_TRUE(context.ResolveService<RenderService>() == nullptr, "解除註冊後解析為 nullptr.", true);
        }

        TEST_MESSAGE("EngineContext type isolation");
        {
            EngineContext context;
            RenderService render;
            PhysicsService physics;
            context.RegisterService<RenderService>(&render);
            context.RegisterService<PhysicsService>(&physics);
            EXPECT_TRUE(context.ResolveService<RenderService>() == &render, "不同型別不衝突.", true);
            EXPECT_TRUE(context.ResolveService<PhysicsService>() == &physics, "不同型別不衝突.", true);
        }

        TEST_MESSAGE("Module-to-module event bus");
        {
            Engine engine;
            EventEmitter emitter;
            EventReceiver receiver;
            engine.GetContext().RegisterService<EventEmitter>(&emitter);
            engine.Attach(&receiver); // OnAttach 內訂閱
            emitter.onPing.Invoke(7);
            EXPECT_TRUE(receiver.received == 7, "接收端收到事件.", true);
            emitter.onPing.Invoke(3);
            EXPECT_TRUE(receiver.received == 10, "事件累加.", true);
            engine.Detach(&receiver); // OnDetach 內退訂
            emitter.onPing.Invoke(100);
            EXPECT_TRUE(receiver.received == 10, "卸載後不再收到事件.", true);
        }

        SUCCESS_MESSAGE("SystemModule");
        return true;
    }
};

#endif // SYSTEM_MODULE_TEST_HPP
