#ifndef FRAMECOUNTER_HPP
#define FRAMECOUNTER_HPP

#include <cstdint>

#include "System/Module/EngineContext.hpp"
#include "System/Module/IModule.hpp"
#include "System/Operation/Event.hpp"

// Frame 計數器:每 tick 產生一個單調遞增的 frame 索引,並透過 typed event 廣播。
//
// - Sim 層確定性(principle #1):純 uint32 計數器,由 FixedUpdate 驅動。
//   不碰 GPU 型別、不碰顯示、不讀牆鐘。
// - 模組只透過 typed event 溝通(principle #4):消費者訂閱 onFrameProduced,
//   完全不耦合生產者是誰。
// - 「Frame 是貨幣」(principle #2):這是 Sim 層的 tick 級 frame 索引。
//   與 FrameClock::frameIndex / tickIndex 用途不同(那是引擎簿記),
//   這裡刻意保持為明確的 uint32,符合 issue 的契約。
//
// 附著順序契約:FrameCounter 必須在任一訂閱者「之前」Attach。
// - 訂閱者的 OnAttach 才能 ResolveService<FrameCounter>() 成功;
// - 其 emission(在 FixedUpdate,依附著順序)先於下游模組執行。
// 這個順序本身是確定性契約的一部分。
struct OnFrameProduced
{
    uint32_t frameIndex;
};

class FrameCounter : public IModule
{
  public:
    Event<void(OnFrameProduced)> onFrameProduced;

    uint32_t GetFrameIndex() const
    {
        return counter;
    }

    void OnAttach(EngineContext &context) override
    {
        context.RegisterService<FrameCounter>(this);
    }

    void OnDetach(EngineContext &context) override
    {
        context.UnregisterService<FrameCounter>();
    }

    void FixedUpdate(const FrameClock &clock) override
    {
        (void)clock;
        onFrameProduced.Invoke(OnFrameProduced{counter});
        counter++;
    }

    void Update(const FrameClock &clock) override
    {
        (void)clock;
    }

  private:
    uint32_t counter = 0;
};

#endif // FRAMECOUNTER_HPP