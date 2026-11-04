#ifndef COMPONENT_SYSTEM_HPP
#define COMPONENT_SYSTEM_HPP

#include "Data/DynamicArray.hpp"
#include "Scene/IComponentLifecycle.hpp"

// ComponentSystem:管理所有帶生命週期的組件，按固定順序分派。
// 執行順序(每幀):
//   1. Awake()  (僅第一次，對所有未 Awake 的組件)
//   2. Start()  (僅第一次，對所有 Awake 但未 Start 的組件)
//   3. FixedUpdate() (固定間隔，由呼叫者決定時機)
//   4. Update() (每幀)
//   5. LateUpdate() (每幀，Update 之後)
//   6. OnDestroy() (組件移除前)
class ComponentSystem
{
  public:
    ComponentSystem() = default;
    ~ComponentSystem()
    {
        // 銷毀所有未被手動移除的組件
        for (size_t i = 0; i < mComponents.GetNElements(); i++)
        {
            IComponentLifecycle *p = mComponents[i];
            if (p && !p->IsDestroyed())
                p->OnDestroy();
        }
        mComponents.RemoveAll();
        mAwakePending.RemoveAll();
        mStartPending.RemoveAll();
    }

    ComponentSystem(const ComponentSystem &) = delete;
    ComponentSystem &operator=(const ComponentSystem &) = delete;

    // 註冊組件(ComponentSystem 不擁有指標，呼叫者負責生命週期)。
    void Register(IComponentLifecycle *pComponent)
    {
        if (!pComponent)
            return;
        mComponents.Append(pComponent);
        if (!pComponent->IsAwake())
            mAwakePending.Append(pComponent);
        if (pComponent->IsAwake() && !pComponent->IsStarted())
            mStartPending.Append(pComponent);
    }

    // 移除組件(執行 OnDestroy)。
    void Unregister(IComponentLifecycle *pComponent)
    {
        if (!pComponent)
            return;
        if (!pComponent->IsDestroyed())
        {
            pComponent->OnDestroy();
            pComponent->SetDestroyed(true);
        }
        RemoveFromArray(mComponents, pComponent);
        RemoveFromArray(mAwakePending, pComponent);
        RemoveFromArray(mStartPending, pComponent);
    }

    // 執行所有未 Awake 的組件的 Awake() (僅第一次)。
    void DoAwake()
    {
        for (size_t i = 0; i < mAwakePending.GetNElements(); i++)
        {
            IComponentLifecycle *p = mAwakePending[i];
            if (p && !p->IsAwake() && !p->IsDestroyed())
            {
                p->Awake();
                p->SetAwake(true);
                // Awake 後若未 Start，加入 StartPending
                if (!p->IsStarted())
                    mStartPending.Append(p);
            }
        }
        mAwakePending.RemoveAll();
    }

    // 執行所有未 Start 的組件的 Start() (僅第一次)。
    void DoStart()
    {
        for (size_t i = 0; i < mStartPending.GetNElements(); i++)
        {
            IComponentLifecycle *p = mStartPending[i];
            if (p && p->IsAwake() && !p->IsStarted() && !p->IsDestroyed())
            {
                p->Start();
                p->SetStarted(true);
            }
        }
        mStartPending.RemoveAll();
    }

    // 執行所有已 Start 組件的 FixedUpdate()。
    void DoFixedUpdate(float fixedDeltaTime)
    {
        for (size_t i = 0; i < mComponents.GetNElements(); i++)
        {
            IComponentLifecycle *p = mComponents[i];
            if (p && p->IsStarted() && !p->IsDestroyed())
                p->FixedUpdate(fixedDeltaTime);
        }
    }

    // 執行所有已 Start 組件的 Update()。
    void DoUpdate(float deltaTime)
    {
        for (size_t i = 0; i < mComponents.GetNElements(); i++)
        {
            IComponentLifecycle *p = mComponents[i];
            if (p && p->IsStarted() && !p->IsDestroyed())
                p->Update(deltaTime);
        }
    }

    // 執行所有已 Start 組件的 LateUpdate()。
    void DoLateUpdate(float deltaTime)
    {
        for (size_t i = 0; i < mComponents.GetNElements(); i++)
        {
            IComponentLifecycle *p = mComponents[i];
            if (p && p->IsStarted() && !p->IsDestroyed())
                p->LateUpdate(deltaTime);
        }
    }

    // 取得已註冊組件數量。
    size_t GetComponentCount() const { return mComponents.GetNElements(); }

  private:
    template <class T> void RemoveFromArray(DynamicArray<T *> &arr, T *p)
    {
        for (size_t i = 0; i < arr.GetNElements(); i++)
        {
            if (arr[i] == p)
            {
                size_t last = arr.GetNElements() - 1;
                if (i != last)
                    arr[i] = arr[last]; // 移最後一個到此位置
                arr.RemoveLast();       // 移除最後一個
                return;
            }
        }
    }

    DynamicArray<IComponentLifecycle *> mComponents;
    DynamicArray<IComponentLifecycle *> mAwakePending;
    DynamicArray<IComponentLifecycle *> mStartPending;
};

#endif // COMPONENT_SYSTEM_HPP
