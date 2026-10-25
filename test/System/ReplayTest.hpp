#ifndef SYSTEM_REPLAY_TEST_HPP
#define SYSTEM_REPLAY_TEST_HPP

#include "Test.hpp"
#include "System/Module/Engine.hpp"
#include "System/Module/IModule.hpp"
#include "System/Module/SimInput.hpp"
#include "System/Module/SimRecorder.hpp"
#include "System/World/World.hpp"

#include <cmath>

// WorldTest.hpp 已在同一 TU 定義全域 struct Position — 整個測試收進
// namespace replaytest 隔離,避免與 SystemModule.cpp 的合併編譯衝突。
namespace replaytest
{

struct Position
{
    float x, y, z;
};

struct Velocity
{
    float x, y, z;
};

// 從 EngineContext 讀 SimInput 的確定性系統:
// - bit0 → 沿 X 移動,速度乘 axisX
// - bit1 → 沿 Y 移動,速度乘 axisY
class InputMoveSystem : public IModule
{
  private:
    EngineContext *pContext = nullptr;

  public:
    World world;

    void OnAttach(EngineContext &context) override
    {
        pContext = &context;
    }

    void FixedUpdate(const FrameClock &clock) override
    {
        const float dt = static_cast<float>(clock.fixedDeltaSeconds);
        const SimInput *pInput = pContext ? pContext->ResolveService<SimInput>() : nullptr;
        for (auto [entity, pos, vel] : world.View<Position, Velocity>())
        {
            (void)entity;
            if (pInput && (pInput->actionBits & 0x1u))
                pos->x += vel->x * dt * pInput->axisX;
            if (pInput && (pInput->actionBits & 0x2u))
                pos->y += vel->y * dt * pInput->axisY;
        }
    }
};

// 元件 hash 的決定性:Add 會把整塊儲存清零再指派,所以每個成員都是確定值。
// 注意:隱含 padding 的位元組在 C++ 是 unspecified —— 指派可能被實作成整塊複製,
// 把來源暫時物的 padding 一起帶進來,gcc 與 clang 的行為就不同(#129)。所以這裡
// 宣告「明確」的填充成員:聚合初始化會把沒被指名的成員值初始化(0),這是標準
// 保證的。測試因此驗證引擎真正保證的東西:同值 → 同 hash。
struct PaddedComponent
{
    float value;
    uint8_t tag;
    uint8_t explicitPadding[3]; // 明確填充,取代 compiler 的隱含 padding
};

// ---------------- 測試主體 ----------------

class ReplayTest : public Test
{
  public:
    ReplayTest() : Test("SystemReplay")
    {
    }

    bool Run() noexcept override
    {
        // 腳本化輸入:12 tick,變化涵蓋 bit0/bit1/axisX/axisY/暫停
        static const SimInput script[12] = {
            {0x1u, 1.0f, 0.0f}, {0x1u, 2.0f, 0.0f}, {0x0u, 0.0f, 0.0f},
            {0x1u, 0.5f, 0.0f}, {0x3u, 1.0f, 1.0f}, {0x1u, 1.0f, 0.0f},
            {0x0u, 0.0f, 0.0f}, {0x0u, 0.0f, 0.0f}, {0x1u, 1.5f, 0.0f},
            {0x1u, 1.0f, 0.0f}, {0x2u, 0.0f, 1.0f}, {0x1u, 1.0f, 0.0f},
        };
        const int N = 12;

        TEST_MESSAGE("Record → replay → per-tick state hash equality");
        {
            // ---- live run:腳本寫入輸入,recorder 逐 tick 錄下 ----
            Engine engine(1);
            SimRecorder recorder;
            InputMoveSystem sim;
            engine.Attach(&recorder); // 先附著:輸入先寫,系統後讀
            engine.Attach(&sim);

            Entity e = sim.world.CreateEntity();
            sim.world.AddComponent<Position>(e, Position{0.0f, 0.0f, 0.0f});
            sim.world.AddComponent<Velocity>(e, Velocity{60.0f, 0.0f, 0.0f});

            DynamicArray<uint64_t> liveHashes;
            for (int i = 0; i < N; i++)
            {
                recorder.GetLiveInput() = script[i];
                engine.Tick(1.0 / 60.0);
                liveHashes.Append(sim.world.Hash());
            }
            const uint64_t liveFinal = sim.world.Hash();

            EXPECT_TRUE(recorder.GetInputCount() == static_cast<size_t>(N), "錄到 N 筆輸入.", true);
            EXPECT_TRUE(liveFinal != 0, "hash 非零(狀態有內容).", true);

            // ---- replay run:全新 engine/world,recorder 重播錄音 ----
            Engine replayEngine(1);
            SimRecorder replayRecorder;
            InputMoveSystem replaySim;
            replayRecorder.SetReplaying(true);
            replayRecorder.LoadLog(recorder.GetLog());
            replayEngine.Attach(&replayRecorder);
            replayEngine.Attach(&replaySim);

            Entity r = replaySim.world.CreateEntity();
            replaySim.world.AddComponent<Position>(r, Position{0.0f, 0.0f, 0.0f});
            replaySim.world.AddComponent<Velocity>(r, Velocity{60.0f, 0.0f, 0.0f});

            bool allEqual = true;
            for (int i = 0; i < N; i++)
            {
                replayEngine.Tick(1.0 / 60.0); // 不寫輸入 — recorder 自己餵
                if (replaySim.world.Hash() != liveHashes[static_cast<size_t>(i)])
                    allEqual = false;
            }
            EXPECT_TRUE(allEqual, "每 tick state hash 與 live 相同(確定性 replay).", true);
            EXPECT_TRUE(replaySim.world.Hash() == liveFinal, "最終狀態 hash 相同.", true);
        }

        TEST_MESSAGE("Tampered input → hash diverges (replay 真的吃錄音)");
        {
            Engine engine(1);
            SimRecorder recorder;
            InputMoveSystem sim;
            engine.Attach(&recorder);
            engine.Attach(&sim);

            Entity e = sim.world.CreateEntity();
            sim.world.AddComponent<Position>(e, Position{0.0f, 0.0f, 0.0f});
            sim.world.AddComponent<Velocity>(e, Velocity{60.0f, 0.0f, 0.0f});

            DynamicArray<uint64_t> liveHashes;
            for (int i = 0; i < N; i++)
            {
                recorder.GetLiveInput() = script[i];
                engine.Tick(1.0 / 60.0);
                liveHashes.Append(sim.world.Hash());
            }

            // 篡改錄音:把 tick 4 的 bit0 清掉(0x3 → 0x2)
            DynamicArray<SimInput> tampered = recorder.GetLog();
            tampered[4].actionBits ^= 0x1u;

            Engine replayEngine(1);
            SimRecorder replayRecorder;
            InputMoveSystem replaySim;
            replayRecorder.SetReplaying(true);
            replayRecorder.LoadLog(tampered);
            replayEngine.Attach(&replayRecorder);
            replayEngine.Attach(&replaySim);

            Entity r = replaySim.world.CreateEntity();
            replaySim.world.AddComponent<Position>(r, Position{0.0f, 0.0f, 0.0f});
            replaySim.world.AddComponent<Velocity>(r, Velocity{60.0f, 0.0f, 0.0f});

            bool equalBefore = true, divergedAfter = false;
            for (int i = 0; i < N; i++)
            {
                replayEngine.Tick(1.0 / 60.0);
                const uint64_t h = replaySim.world.Hash();
                if (i < 4)
                {
                    if (h != liveHashes[static_cast<size_t>(i)])
                        equalBefore = false; // 篡改點之前的 tick 必須仍相同
                }
                else if (h != liveHashes[static_cast<size_t>(i)])
                {
                    divergedAfter = true;
                }
            }
            EXPECT_TRUE(equalBefore, "篡改點前 hash 相同.", true);
            EXPECT_TRUE(divergedAfter, "篡改點後 hash 分歧(證明 replay 吃錄音).", true);
        }

        TEST_MESSAGE("Padding 元件:零初始化讓 hash 確定");
        {
            World w1, w2;
            Entity a = w1.CreateEntity();
            w1.AddComponent<PaddedComponent>(a, PaddedComponent{1.5f, 7});
            Entity b = w2.CreateEntity();
            w2.AddComponent<PaddedComponent>(b, PaddedComponent{1.5f, 7});
            EXPECT_TRUE(w1.Hash() == w2.Hash(), "含 padding 的元件 hash 相同(padding 已歸零).", true);

            // 換值 → hash 必須不同
            w2.GetComponent<PaddedComponent>(b)->value = 2.5f;
            EXPECT_TRUE(w1.Hash() != w2.Hash(), "值變 → hash 不同.", true);
        }

        SUCCESS_MESSAGE("SystemReplay");
        return true;
    }
};

} // namespace replaytest

#endif // SYSTEM_REPLAY_TEST_HPP
