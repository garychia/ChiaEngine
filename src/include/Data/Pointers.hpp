#ifndef POINTERS_HPP
#define POINTERS_HPP

#include <atomic>
#include <cstddef>
#include <mutex>

template <class T> class SharedPtr;

template <class T> class WeakPtr;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100) // 某些靜態 helper 未使用的參數(MSVC
#endif

// ---------------------------------------------------------------------------
// Ptr:獨佔所有權的 raw pointer(RAII)。
//
// #86:複製建構/複製賦值一律 delete。過去 compiler 隱式 copy ctor 淺拷貝 rawPtr
// (→ 兩個 owner → 雙 delete),而 copy-assign 卻做「數值複製(*rawPtr = *other)」,
// 兩者語意不一致正是 hazard 來源。要共享所有權 → SharedPtr;要觀測 → WeakPtr。
// ---------------------------------------------------------------------------
template <class T> class Ptr
{
  protected:
    T *rawPtr;

  public:
    using ValueType = T;

    template <class... Args> static Ptr<T> Construct(Args... args)
    {
        Ptr<T> ptr;
        ptr.rawPtr = new T(args...);
        return ptr;
    }

    template <class Subclass, class... Args> static Ptr<T> Construct(Args... args)
    {
        Ptr<T> ptr;
        ptr.rawPtr = dynamic_cast<Subclass *>(new Subclass(args...));
        return ptr;
    }

    Ptr() : rawPtr(nullptr)
    {
    }

    template <class U> Ptr(Ptr<U> &&other) : rawPtr(static_cast<T *>(other.rawPtr))
    {
        other.rawPtr = nullptr;
    }

    // #86:獨佔所有權不可複製 — 杜絕雙 delete。
    Ptr(const Ptr<T> &) = delete;

    Ptr<T> &operator=(const Ptr<T> &) = delete;

    ~Ptr()
    {
        Release();
    }

    bool IsValid() const
    {
        return rawPtr != nullptr;
    }

    void Release()
    {
        if (rawPtr)
            delete rawPtr;
        rawPtr = nullptr;
    }

    T *GetRaw()
    {
        return rawPtr;
    }

    const T *GetRaw() const
    {
        return rawPtr;
    }

    Ptr &operator=(Ptr<T> &&other)
    {
        if (IsValid())
            Release();
        rawPtr = other.rawPtr;
        other.rawPtr = nullptr;
        return *this;
    }

    T &operator*()
    {
        return *rawPtr;
    }

    const T &operator*() const
    {
        return *rawPtr;
    }

    T *operator->()
    {
        return rawPtr;
    }

    const T *operator->() const
    {
        return rawPtr;
    }

    operator bool() const
    {
        return IsValid();
    }

    friend class SharedPtr<T>;
};

// PtrInfo 維持在 global scope(非 anonymous namespace):
// SharedPtr<T> 是模板、每個 TU 各自實例化,subobject 的型別若藏在 anonymous namespace
// 會觸發 -Wsubobject-linkage(每個 TU 拿到的型別不同 → 潛在 ODR violation)。
//
// #86 引用計數改為 std::atomic:
//   * 一般 ++ / --:memory_order_relaxed(單一計數,無順序依賴需求)
//   * 最後一份釋放:memory_order_acq_rel(讓其他執行緒清楚看見「最後釋放已完成」)
//   * WeakPtr::Lock 升級與「最後釋放銷毀」這兩種「檢查後再決定」的過渡,atomic 無法原子涵蓋,
//     以互斥 mutex 保護(只有路徑衝突時才碰到,普通複製/釋放不走它)。
//
// 生命週期規約(#86):
//   * 物件(ptr)在 sharedCount 歸零「當下」即銷毀 —— 即使還有 WeakPtr 存活 →
//     WeakPtr::Lock 對已亡物件回空(不再讓物件苟延殘喘、也不再可用驗證過期的指標)。
//   * 控制塊(PtrInfo)由 weakCount 決定存亡。weakCount 初始 = 1(phantom,
//     代表「strong 擁有群」的存在);strong 歸零時釋放 phantom,控制塊最後由
//     「把 weakCount 減到 0 的那一位」刪除 → 恰好一次、無 UAF(boost/std 同款手法)。
class PtrInfo
{
  private:
    std::atomic<size_t> sharedCount;
    std::atomic<size_t> weakCount; // 含 1 個 phantom(strong 擁有群)
    std::atomic<bool> objectDestroyed;
    void *ptr;
    void (*deleter)(void *);
    std::mutex mutex; // 保護 TryLock 與「最後一份釋放 → 銷毀」的互斥

  public:
    PtrInfo(void *inPtr, void (*inDeleter)(void *))
        : sharedCount(1), weakCount(1), objectDestroyed(false), ptr(inPtr), deleter(inDeleter), mutex()
    {
    }

    ~PtrInfo()
    {
        // 防呆:正常路徑下物件會在 sharedCount 歸零時由 DecrementSharedCounter 銷毀
        // 並清空 ptr;這裡只補「異常路徑(物件從未釋放)」的尾巴。
        if (ptr)
            deleter(ptr);
    }

    template <class T> T *GetPtr()
    {
        return static_cast<T *>(ptr);
    }

    template <class T> const T *GetPtr() const
    {
        return static_cast<const T *>(ptr);
    }

    // 診斷用:目前 strong 引用數(relaxed,非同步保證)。
    size_t GetSharedCount() const
    {
        return sharedCount.load(std::memory_order_relaxed);
    }

    void IncrementSharedCounter()
    {
        sharedCount.fetch_add(1, std::memory_order_relaxed);
    }

    void IncrementWeakCounter()
    {
        weakCount.fetch_add(1, std::memory_order_relaxed);
    }

    // 最後一份 strong 釋放 → 立即銷毀物件(即使還有 weak);再釋放 phantom weak。
    void DecrementSharedCounter()
    {
        if (sharedCount.fetch_sub(1, std::memory_order_acq_rel) != 1)
            return;
        {
            std::lock_guard<std::mutex> lock(mutex);
            deleter(ptr);
            ptr = nullptr;
            objectDestroyed.store(true, std::memory_order_release);
        }
        // phantom weak:shared 歸零代表 strong 擁有群結束,把初始那 1 個額外 weak 還掉。
        if (weakCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
            delete this;
    }

    void DecrementWeakCounter()
    {
        if (weakCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
            delete this;
    }

    // #86:WeakPtr::Lock / SharedPtr-from-WeakPtr 的安全升級。
    // 與「最後釋放 → 銷毀」互斥(mutex):物件已銷毀、或計數已歸零(最後釋放進行中)→
    // 回 false,絕不復活已亡物件、也不讓升級與銷毀交錯。
    bool TryLock()
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (objectDestroyed.load(std::memory_order_relaxed))
            return false;
        if (sharedCount.load(std::memory_order_relaxed) == 0)
            return false;
        sharedCount.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    bool IsValid() const
    {
        return sharedCount.load(std::memory_order_acquire) > 0;
    }
};

template <class T> void PtrInfoDeleteTyped(void *p)
{
    delete static_cast<T *>(p);
}

// ---------------------------------------------------------------------------
// SharedPtr:共享所有權(atomic 引用計數)。copy/move 跨執行緒安全。
// #86:CleanUp/IncrementCounter 不再 virtual(WeakPtr 已獨立,不再繼承);
// 從 WeakPtr 升級一律走 TryLock(目標已亡 → 得到空 SharedPtr,不懸空)。
// ---------------------------------------------------------------------------
template <class T> class SharedPtr
{
  private:
    PtrInfo *pInfo;

    void CleanUp()
    {
        if (!pInfo)
            return;
        pInfo->DecrementSharedCounter();
        pInfo = nullptr;
    }

  public:
    using ValueType = T;

    template <class... Args> static SharedPtr<T> Construct(Args... args)
    {
        SharedPtr<T> ptr;
        ptr.pInfo = new PtrInfo(new T(args...), &PtrInfoDeleteTyped<T>);
        return ptr;
    }

    template <class SubclassType, class... Args> static SharedPtr<T> Construct(Args... args)
    {
        SharedPtr<T> ptr;
        ptr.pInfo = new PtrInfo(static_cast<void *>(static_cast<T *>(new SubclassType(args...))),
                                &PtrInfoDeleteTyped<SubclassType>);
        return ptr;
    }

    SharedPtr() : pInfo(nullptr)
    {
    }

    SharedPtr(const SharedPtr<T> &other) : pInfo(other.pInfo)
    {
        IncrementCounter();
    }

    template <class U> SharedPtr(const SharedPtr<U> &other) : pInfo(other.pInfo)
    {
        IncrementCounter();
    }

    // #86:從 WeakPtr 安全升級(Lock)。目標已亡 → 空 SharedPtr(不再無條件取碼/懸空)。
    SharedPtr(const WeakPtr<T> &other) : pInfo(nullptr)
    {
        if (other.pInfo && other.pInfo->TryLock())
            pInfo = other.pInfo;
    }

    SharedPtr(SharedPtr<T> &&other) noexcept : pInfo(other.pInfo)
    {
        other.pInfo = nullptr;
    }

    SharedPtr(WeakPtr<T> &&other) : pInfo(nullptr)
    {
        if (other.pInfo && other.pInfo->TryLock())
            pInfo = other.pInfo;
        other.Clear();
    }

    SharedPtr(Ptr<T> &&other) : pInfo(nullptr)
    {
        if (!other.IsValid())
            return;
        pInfo = new PtrInfo(other.GetRaw(), &PtrInfoDeleteTyped<T>);
        other.rawPtr = nullptr;
    }

    ~SharedPtr()
    {
        CleanUp();
    }

    bool IsValid() const
    {
        return pInfo && pInfo->IsValid();
    }

    // 診斷用 strong 引用數(relaxed)。
    size_t GetRefCount() const
    {
        return pInfo ? pInfo->GetSharedCount() : 0;
    }

    T *GetRaw()
    {
        return IsValid() ? pInfo->template GetPtr<T>() : nullptr;
    }

    const T *GetRaw() const
    {
        return IsValid() ? pInfo->template GetPtr<T>() : nullptr;
    }

    void Release()
    {
        CleanUp();
    }

    operator bool() const
    {
        return IsValid();
    }

    SharedPtr<T> &operator=(const SharedPtr<T> &ref)
    {
        if (pInfo == ref.pInfo)
            return *this;
        CleanUp();
        pInfo = ref.pInfo;
        IncrementCounter();
        return *this;
    }

    SharedPtr<T> &operator=(SharedPtr<T> &&ref) noexcept
    {
        if (pInfo == ref.pInfo)
        {
            ref.pInfo = nullptr;
            return *this;
        }
        CleanUp();
        pInfo = ref.pInfo;
        ref.pInfo = nullptr;
        return *this;
    }

    bool operator==(const SharedPtr<T> &other) const
    {
        return pInfo == other.pInfo;
    }

    bool operator==(const WeakPtr<T> &other) const
    {
        return pInfo == other.pInfo;
    }

    T &operator*()
    {
        return *pInfo->template GetPtr<T>();
    }

    const T &operator*() const
    {
        return *pInfo->template GetPtr<T>();
    }

    T *operator->()
    {
        return IsValid() ? pInfo->template GetPtr<T>() : nullptr;
    }

    const T *operator->() const
    {
        return IsValid() ? pInfo->template GetPtr<T>() : nullptr;
    }

    void IncrementCounter()
    {
        if (pInfo)
            pInfo->IncrementSharedCounter();
    }

    template <class U> friend class SharedPtr;

    friend class WeakPtr<T>;
};

// ---------------------------------------------------------------------------
// WeakPtr:不持有所有權的觀測指標(#86 起獨立實作,不再繼承 SharedPtr)。
//
// 安全的唯一入口是 Lock()—— 回傳 SharedPtr(Lock 與最後釋放互斥,絕不懸空解引用)。
// operator*/->/bool 已移除:它們的「檢查後解引用」視窗,正是物件可被另一執行緒
// 釋放的洞。IsValid() 只供診斷(可能馬上過期),要用一律先 Lock()。
// ---------------------------------------------------------------------------
template <class T> class WeakPtr
{
  private:
    PtrInfo *pInfo;

    void IncrementWeakCounter()
    {
        if (pInfo)
            pInfo->IncrementWeakCounter();
    }

  public:
    using ValueType = T;

    WeakPtr() : pInfo(nullptr)
    {
    }

    WeakPtr(const WeakPtr<T> &other) : pInfo(other.pInfo)
    {
        IncrementWeakCounter();
    }

    template <class U> WeakPtr(const WeakPtr<U> &other) : pInfo(other.pInfo)
    {
        IncrementWeakCounter();
    }

    WeakPtr(WeakPtr<T> &&other) noexcept : pInfo(other.pInfo)
    {
        other.pInfo = nullptr;
    }

    WeakPtr(const SharedPtr<T> &other) : pInfo(other.pInfo)
    {
        IncrementWeakCounter();
    }

    template <class U> WeakPtr(const SharedPtr<U> &other) : pInfo(other.pInfo)
    {
        IncrementWeakCounter();
    }

    ~WeakPtr()
    {
        Clear();
    }

    WeakPtr<T> &operator=(const WeakPtr<T> &other)
    {
        if (pInfo == other.pInfo)
            return *this;
        Clear();
        pInfo = other.pInfo;
        IncrementWeakCounter();
        return *this;
    }

    WeakPtr<T> &operator=(WeakPtr<T> &&other) noexcept
    {
        if (pInfo == other.pInfo)
        {
            other.pInfo = nullptr;
            return *this;
        }
        Clear();
        pInfo = other.pInfo;
        other.pInfo = nullptr;
        return *this;
    }

    WeakPtr<T> &operator=(const SharedPtr<T> &other)
    {
        if (pInfo == other.pInfo)
            return *this;
        Clear();
        pInfo = other.pInfo;
        IncrementWeakCounter();
        return *this;
    }

    // #86:安全存取唯一通道。目標存活 → 升級為 SharedPtr;已亡 → 空 SharedPtr。
    SharedPtr<T> Lock() const
    {
        SharedPtr<T> out;
        if (pInfo && pInfo->TryLock())
            out.pInfo = pInfo;
        return out;
    }

    bool IsValid() const
    {
        return pInfo && pInfo->IsValid();
    }

    void Release()
    {
        Clear();
    }

    void Clear()
    {
        if (!pInfo)
            return;
        pInfo->DecrementWeakCounter();
        pInfo = nullptr;
    }

    bool operator==(const WeakPtr<T> &other) const
    {
        return pInfo == other.pInfo;
    }

    bool operator==(const SharedPtr<T> &other) const
    {
        return pInfo == other.pInfo;
    }

    template <class U> friend class SharedPtr;

    template <class U> friend class WeakPtr;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif