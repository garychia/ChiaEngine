#ifndef SIM_RECORDER_HPP
#define SIM_RECORDER_HPP

#include "Data/DynamicArray.hpp"
#include "SimInput.hpp"
#include "System/Module/EngineContext.hpp"
#include "System/Module/IModule.hpp"

// 輸入錄製 / 重播器。
//
// - 擁有 SimInput 並註冊為 EngineContext 服務:Sim 系統在 FixedUpdate 裡
//   ResolveService<SimInput>() 讀輸入,不直接耦合輸入來源。
// - Live 模式:每 tick 把目前輸入複製進 log[tickIndex]。
// - Replay 模式:每 tick 用 log[tickIndex] 覆寫服務 — 系統讀到與 live 完全
//   相同的輸入序列,配合 World::Hash() 即可驗證確定性重播。
//
// 執行順序 = 附著順序:SimRecorder 必須在所有讀輸入的模組之前 Attach,
// 這樣它先寫/還原輸入,系統後讀。這個順序本身是確定性契約的一部分。
class SimRecorder : public IModule
{
  private:
    SimInput input;             // 註冊為 context service(live 的寫入點)
    DynamicArray<SimInput> log; // 每 tick 一筆
    bool replaying;

  public:
    SimRecorder() : input(), log(), replaying(false)
    {
    }

    // live 模式:寫入當下輸入(由 View 層或測試驅動)
    SimInput &GetLiveInput()
    {
        return input;
    }

    void SetReplaying(bool value)
    {
        replaying = value;
    }

    bool IsReplaying() const
    {
        return replaying;
    }

    // 錄製結果存取(序列化 / 餵給另一個 recorder)
    const DynamicArray<SimInput> &GetLog() const
    {
        return log;
    }

    size_t GetInputCount() const
    {
        return log.GetNElements();
    }

    const SimInput &GetInputAt(size_t index) const
    {
        return log[index];
    }

    // 載入外部錄音(replay 用);清掉現有 log
    void LoadLog(const DynamicArray<SimInput> &source)
    {
        log = source;
    }

    void ClearLog()
    {
        log = DynamicArray<SimInput>();
    }

    void OnAttach(EngineContext &context) override
    {
        context.RegisterService<SimInput>(&input);
    }

    void OnDetach(EngineContext &context) override
    {
        context.UnregisterService<SimInput>();
    }

    void FixedUpdate(const FrameClock &clock) override
    {
        if (replaying)
        {
            // 重播:還原該 tick 的輸入。錄音比執行短 → 用空白輸入(保持確定性)。
            const size_t tick = static_cast<size_t>(clock.tickIndex);
            input = (tick < log.GetNElements()) ? log[tick] : SimInput();
        }
        else
        {
            log.Append(input);
        }
    }

    void Update(const FrameClock &clock) override
    {
        (void)clock;
    }
};

#endif // SIM_RECORDER_HPP
