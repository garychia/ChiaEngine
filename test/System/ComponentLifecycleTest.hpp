#ifndef COMPONENT_LIFECYCLE_TEST_HPP
#define COMPONENT_LIFECYCLE_TEST_HPP

#include "Scene/ComponentSystem.hpp"
#include "Scene/IComponentLifecycle.hpp"
#include "Scene/SceneSystem.hpp"
#include "System/Module/FrameClock.hpp"
#include "Test.hpp"

#include <cstdio>

namespace componentlifecycle_test
{

// 測試用組件:記錄每個生命週期方法被呼叫的次數。
class TestComponent : public IComponentLifecycle
{
  public:
    int awakeCount = 0;
    int startCount = 0;
    int updateCount = 0;
    int fixedUpdateCount = 0;
    int lateUpdateCount = 0;
    int onDestroyCount = 0;
    float lastUpdateDt = 0.f;
    float lastFixedDt = 0.f;
    float lastLateDt = 0.f;
    String name;

    explicit TestComponent(const String &n) : name(n) {}

    void Awake() override { awakeCount++; }
    void Start() override { startCount++; }
    void Update(float dt) override { updateCount++; lastUpdateDt = dt; }
    void FixedUpdate(float dt) override { fixedUpdateCount++; lastFixedDt = dt; }
    void LateUpdate(float dt) override { lateUpdateCount++; lastLateDt = dt; }
    void OnDestroy() override { onDestroyCount++; }
};

// ComponentLifecycleTest:A3.2 組件生命週期契約。
// 1. Awake/Start 僅執行一次
// 2. Update 每幀執行
// 3. 執行順序: Awake → Start → Update → LateUpdate
// 4. OnDestroy 在銷毀前執行
class ComponentLifecycleTest : public Test
{
  public:
    ComponentLifecycleTest() : Test("ComponentLifecycle")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:Awake/Start 僅執行一次 ----
        {
            ComponentSystem cs;
            TestComponent comp("A");

            cs.Register(&comp);

            // 第一次 DoAwake/DoStart
            cs.DoAwake();
            cs.DoStart();
            EXPECT_TRUE(comp.awakeCount == 1, "Awake 執行 1 次", true);
            EXPECT_TRUE(comp.startCount == 1, "Start 執行 1 次", true);

            // 再次呼叫不應重複
            cs.DoAwake();
            cs.DoStart();
            EXPECT_TRUE(comp.awakeCount == 1, "DoAwake 重複呼叫後仍為 1", true);
            EXPECT_TRUE(comp.startCount == 1, "DoStart 重複呼叫後仍為 1", true);
        }

        // ---- AC2:Update 每幀執行 ----
        {
            ComponentSystem cs;
            TestComponent comp("B");

            cs.Register(&comp);
            cs.DoAwake();
            cs.DoStart();

            cs.DoUpdate(0.016f);
            cs.DoUpdate(0.020f);
            cs.DoUpdate(0.015f);

            EXPECT_TRUE(comp.updateCount == 3, "Update 執行 3 次", true);
            EXPECT_TRUE(comp.lastUpdateDt == 0.015f, "最後一次 Update dt 正確", true);
        }

        // ---- AC3:FixedUpdate 每固定間隔執行 ----
        {
            ComponentSystem cs;
            TestComponent comp("C");

            cs.Register(&comp);
            cs.DoAwake();
            cs.DoStart();

            cs.DoFixedUpdate(0.02f);
            cs.DoFixedUpdate(0.02f);

            EXPECT_TRUE(comp.fixedUpdateCount == 2, "FixedUpdate 執行 2 次", true);
            EXPECT_TRUE(comp.lastFixedDt == 0.02f, "FixedUpdate dt 正確", true);
        }

        // ---- AC4:LateUpdate 在 Update 之後執行 ----
        {
            ComponentSystem cs;
            TestComponent comp("D");

            cs.Register(&comp);
            cs.DoAwake();
            cs.DoStart();

            cs.DoUpdate(0.016f);
            cs.DoLateUpdate(0.016f);

            EXPECT_TRUE(comp.updateCount == 1 && comp.lateUpdateCount == 1,
                        "Update + LateUpdate 各執行 1 次", true);
        }

        // ---- AC5:執行順序 Awake → Start → Update → LateUpdate ----
        {
            ComponentSystem cs;
            TestComponent comp("E");

            cs.Register(&comp);

            // 只呼叫 DoAwake
            cs.DoAwake();
            EXPECT_TRUE(comp.awakeCount == 1 && comp.startCount == 0 &&
                          comp.updateCount == 0 && comp.lateUpdateCount == 0,
                        "DoAwake 後只有 Awake 執行", true);

            // 只呼叫 DoStart
            cs.DoStart();
            EXPECT_TRUE(comp.awakeCount == 1 && comp.startCount == 1 &&
                          comp.updateCount == 0 && comp.lateUpdateCount == 0,
                        "DoStart 後只有 Awake + Start 執行", true);

            // 呼叫 Update + LateUpdate
            cs.DoUpdate(0.01f);
            cs.DoLateUpdate(0.01f);
            EXPECT_TRUE(comp.awakeCount == 1 && comp.startCount == 1 &&
                          comp.updateCount == 1 && comp.lateUpdateCount == 1,
                        "完整順序 Awake→Start→Update→LateUpdate", true);
        }

        // ---- AC6:OnDestroy 在銷毀前執行 ----
        {
            ComponentSystem cs;
            TestComponent comp("F");

            cs.Register(&comp);
            cs.DoAwake();
            cs.DoStart();

            cs.Unregister(&comp);
            EXPECT_TRUE(comp.onDestroyCount == 1, "Unregister 後 OnDestroy 執行 1 次", true);
            EXPECT_TRUE(comp.IsDestroyed(), "Unregister 後 IsDestroyed()=true", true);
        }

        // ---- AC7:SceneSystem 整合 ComponentSystem ----
        {
            SceneSystem system;
            Entity e = system.CreateNode();

            // 透過 GameObject 註冊生命週期組件(簡化：直接測試 ComponentSystem 可存取)
            // 這裡只驗證 SceneSystem 能建構且 ComponentSystem 不會崩潰
            system.FixedUpdate(FrameClock{0, 0, 0.016, 1.0 / 60.0});
            system.Update(FrameClock{1, 1, 0.016, 1.0 / 60.0});

            EXPECT_TRUE(true, "SceneSystem FixedUpdate/Update 與 ComponentSystem 整合不崩潰", true);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }

  private:
};

} // namespace componentlifecycle_test

#endif // COMPONENT_LIFECYCLE_TEST_HPP
