#ifndef PHYSICS_SYSTEM_HPP
#define PHYSICS_SYSTEM_HPP

#include <cstdint>

#include "Physics/ColliderComponent.hpp"
#include "Physics/ColliderHandler.hpp"
#include "System/Module/EngineContext.hpp"
#include "System/Module/IModule.hpp"
#include "System/Operation/Event.hpp"
#include "System/World/ComponentPool.hpp"
#include "System/World/Entity.hpp"
#include "System/World/World.hpp"

// 碰撞事件:成對 Entity,以 entityIndex 標準化(a.index < b.index,無 self-pair)。
struct OnCollision
{
    Entity a;
    Entity b;
};

// 碰撞系統(Sim 層,確定性,無 GPU/牆鐘):
// - 擁有自己的 World,固定步進驅動,逐 tick 對所有成對 ColliderComponent 做
//   narrow-phase overlap,命中即廣播 typed event(principle #4)。
// - 確定性契約:遍歷順序 = dense ComponentPool 陣列順序的巢狀 for(i < j),
//   每對恰好算一次、以固定 canonical 順序發出 → 相同輸入 → 相同碰撞序列。
// - 不做 broad-phase(spatial-hash 會引入 iteration-order 相依,破壞確定性)。
class PhysicsSystem : public IModule
{
  public:
    World world;
    Event<void(OnCollision)> onCollision;

    void OnAttach(EngineContext &context) override
    {
        context.RegisterService<PhysicsSystem>(this);
    }

    void OnDetach(EngineContext &context) override
    {
        (void)context;
    }

    void FixedUpdate(const FrameClock &clock) override
    {
        (void)clock;
        ComponentPool<ColliderComponent> *pPool = world.GetPool<ColliderComponent>();
        if (!pPool)
            return;
        const uint32_t n = pPool->GetNElements();
        for (uint32_t i = 0; i < n; i++)
        {
            const uint32_t ownerI = pPool->OwnerAt(i);
            const ColliderComponent *pA = pPool->Get(ownerI);
            for (uint32_t j = i + 1; j < n; j++)
            {
                const uint32_t ownerJ = pPool->OwnerAt(j);
                if (!ColliderHandler::Overlaps(*pA, *pPool->Get(ownerJ)))
                    continue;
                const Entity a = world.GetEntityByIndex(ownerI);
                const Entity b = world.GetEntityByIndex(ownerJ);
                if (a.GetIndex() <= b.GetIndex())
                    onCollision.Invoke(OnCollision{a, b});
                else
                    onCollision.Invoke(OnCollision{b, a});
            }
        }
    }

    void Update(const FrameClock &clock) override
    {
        (void)clock;
    }
};

#endif // PHYSICS_SYSTEM_HPP
