#ifndef I_COMPONENT_LIFECYCLE_HPP
#define I_COMPONENT_LIFECYCLE_HPP

#include <cstdint>

// IComponentLifecycle:組件生命週期介面。
// 實作此介面的組件會由 ComponentSystem 按固定順序分派：
//   Awake() → Start() → FixedUpdate() → Update() → LateUpdate() → OnDestroy()
// Awake/Start 僅執行一次；FixedUpdate/Update/LateUpdate 每幀/固定間隔執行；
// OnDestroy 在組件從 ComponentSystem 移除前執行。
class IComponentLifecycle
{
  public:
    virtual ~IComponentLifecycle() = default;

    // 初始化(僅執行一次，在 Start 之前)。
    virtual void Awake()
    {
    }

    // 啟動(僅執行一次，在 Awake 之後，第一個 Update 之前)。
    virtual void Start()
    {
    }

    // 每幀執行(非確定性，視渲染幀率)。
    virtual void Update(float deltaTime)
    {
        (void)deltaTime;
    }

    // 固定時間間隔執行(確定性，物理/模擬用)。
    virtual void FixedUpdate(float fixedDeltaTime)
    {
        (void)fixedDeltaTime;
    }

    // 每幀 Update 之後執行(攝影機跟隨、後處理等)。
    virtual void LateUpdate(float deltaTime)
    {
        (void)deltaTime;
    }

    // 組件銷毀前執行(釋放資源、取消訂閱)。
    virtual void OnDestroy()
    {
    }

    // 是否已初始化(Awake 已執行)。
    bool IsAwake() const { return mAwake; }

    // 是否已啟動(Start 已執行)。
    bool IsStarted() const { return mStarted; }

    // 是否已銷毀(OnDestroy 已執行)。
    bool IsDestroyed() const { return mDestroyed; }

  public:
    void SetAwake(bool v = true) { mAwake = v; }
    void SetStarted(bool v = true) { mStarted = v; }
    void SetDestroyed(bool v = true) { mDestroyed = v; }

  private:
    bool mAwake = false;
    bool mStarted = false;
    bool mDestroyed = false;
};

#endif // I_COMPONENT_LIFECYCLE_HPP
