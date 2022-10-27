#ifndef POINTERS_HPP
#define POINTERS_HPP

template <class T> class SharedPtr;

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

    template <class U> Ptr(Ptr<U> &&other) : rawPtr(dynamic_cast<T *>(other.rawPtr))
    {
        other.rawPtr = nullptr;
    }

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

    Ptr &operator=(const Ptr<T> &other)
    {
        if (IsValid() && other.IsValid())
            *rawPtr = *other.rawPtr;
        return *this;
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

namespace
{
class PtrInfo
{
  private:
    size_t counter;

    size_t weakCount;

    void *ptr;

  public:
    PtrInfo(void *ptr) : counter(1), weakCount(0), ptr(ptr)
    {
    }

    ~PtrInfo()
    {
        if (ptr)
            delete ptr;
    }

    template <class T> T *GetPtr()
    {
        return (T *)ptr;
    }

    template <class T> const T *GetPtr() const
    {
        return (T *)ptr;
    }

    void IncrementSharedCounter()
    {
        counter++;
    }

    void IncrementWeakCounter()
    {
        weakCount++;
    }

    void DecrementSharedCounter()
    {
        counter--;
    }

    void DecrementWeakCounter()
    {
        weakCount--;
    }

    bool IsValid() const
    {
        return counter > 0 && ptr;
    }

    bool IsDestroyable() const
    {
        return !counter && !weakCount;
    }
};
} // namespace

template <class T> class WeakPtr;

template <class T> class SharedPtr
{
  protected:
    PtrInfo *pInfo;

    virtual void CleanUp()
    {
        if (!pInfo)
            return;
        pInfo->DecrementSharedCounter();
        if (pInfo->IsDestroyable())
            delete pInfo;
        pInfo = nullptr;
    }

    virtual void IncrementCounter()
    {
        if (pInfo)
            pInfo->IncrementSharedCounter();
    }

  public:
    using ValueType = T;

    template <class... Args> static SharedPtr<T> Construct(Args... args)
    {
        SharedPtr<T> ptr;
        ptr.pInfo = new PtrInfo(new T(args...));
        return ptr;
    }

    template <class SubclassType, class... Args> static SharedPtr<T> Construct(Args... args)
    {
        SharedPtr<T> ptr;
        ptr.pInfo = new PtrInfo((T *)new SubclassType(args...));
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

    SharedPtr(const WeakPtr<T> &other) : pInfo(other.pInfo)
    {
        IncrementCounter();
    }

    SharedPtr(SharedPtr<T> &&other) noexcept : pInfo(other.pInfo)
    {
        other.pInfo = nullptr;
    }

    SharedPtr(WeakPtr<T> &&other)
    {
        pInfo = other.pInfo;
        IncrementCounter();
        other.CleanUp();
    }

    SharedPtr(Ptr<T> &&other) : pInfo(nullptr)
    {
        if (!other.IsValid())
            return;
        pInfo = new PtrInfo(other.GetRaw());
        other.rawPtr = nullptr;
    }

    virtual ~SharedPtr()
    {
        CleanUp();
    }

    bool IsValid() const
    {
        return pInfo && pInfo->IsValid();
    }

    T *GetRaw()
    {
        return IsValid() ? pInfo->GetPtr<T>() : nullptr;
    }

    const T *GetRaw() const
    {
        return IsValid() ? pInfo->GetPtr<T>() : nullptr;
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
        CleanUp();
        pInfo = ref.pInfo;
        IncrementCounter();
        return *this;
    }

    SharedPtr<T> &operator=(SharedPtr<T> &&ref) noexcept
    {
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
        return *pInfo->GetPtr<T>();
    }

    const T &operator*() const
    {
        return *pInfo->GetPtr<T>();
    }

    T *operator->()
    {
        return pInfo->GetPtr<T>();
    }

    const T *operator->() const
    {
        return pInfo->GetPtr<T>();
    }

    template <class U> friend class SharedPtr;

    friend class WeakPtr<T>;
};

template <class T> class WeakPtr : public SharedPtr<T>
{
  private:
    void CleanUp() override
    {
        if (!this->pInfo)
            return;
        this->pInfo->DecrementWeakCounter();
        if (this->pInfo->IsDestroyable())
            delete this->pInfo;
        this->pInfo = nullptr;
    }

    void IncrementCounter() override
    {
        if (this->pInfo)
            this->pInfo->IncrementWeakCounter();
    }

  public:
    WeakPtr() : SharedPtr<T>()
    {
    }

    WeakPtr(const WeakPtr<T> &other)
    {
        this->pInfo = other.pInfo;
        IncrementCounter();
    }

    WeakPtr(WeakPtr<T> &&other)
    {
        this->pInfo = other.pInfo;
        other.pInfo = nullptr;
    }

    WeakPtr(const SharedPtr<T> &other)
    {
        this->pInfo = other.pInfo;
        IncrementCounter();
    }

    WeakPtr(SharedPtr<T> &&other)
    {
        this->pInfo = other.pInfo;
        IncrementCounter();
        other.CleanUp();
    }

    ~WeakPtr()
    {
        CleanUp();
    }

    WeakPtr<T> &operator=(const WeakPtr<T> &other)
    {
        CleanUp();
        this->pInfo = other.pInfo;
        IncrementCounter();
        return *this;
    }

    WeakPtr<T> &operator=(WeakPtr<T> &&other) noexcept
    {
        CleanUp();
        this->pInfo = other.pInfo;
        other.pInfo = nullptr;
        return *this;
    }

    T &operator*()
    {
        return *this->pInfo->GetPtr<T>();
    }

    const T &operator*() const
    {
        return *this->pInfo->GetPtr<T>();
    }

    T *operator->()
    {
        return this->pInfo->GetPtr<T>();
    }

    const T *operator->() const
    {
        return this->pInfo->GetPtr<T>();
    }

    friend class SharedPtr<T>;
};

#endif
