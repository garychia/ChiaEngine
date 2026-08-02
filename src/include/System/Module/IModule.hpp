#ifndef IMODULE_HPP
#define IMODULE_HPP

#include "FrameClock.hpp"

class EngineContext;

// 引擎的一切都是 Module。
// - OnAttach : 掛載時註冊服務、訂閱事件
// - FixedUpdate : 固定步進,Sim 模組的心跳,保證確定性
// - Update      : 每幀更新,允許非確定性(View / Platform 模組用)
// - OnDetach    : 卸載時退訂、釋放
// 模組之間不直接互相呼叫,只透過 EngineContext 與事件匯流排溝通。
class IModule
{
  public:
    virtual ~IModule() = default;

    virtual void OnAttach(EngineContext &context)
    {
        (void)context;
    }

    virtual void OnDetach(EngineContext &context)
    {
        (void)context;
    }

    virtual void FixedUpdate(const FrameClock &clock)
    {
        (void)clock;
    }

    virtual void Update(const FrameClock &clock)
    {
        (void)clock;
    }
};

#endif // IMODULE_HPP
