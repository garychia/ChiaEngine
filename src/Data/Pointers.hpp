#ifndef POINTERS_HPP
#define POINTERS_HPP

template <class T> class Ptr
{
  protected:
    T *rawPtr;

  public:
    using ValueType = T;

    template <class... Args> Ptr(const Args &...args) : rawPtr(new T(args...))
    {
    }

    template <class... Args> Ptr(const Ptr<T> &other, Args... args) : rawPtr(nullptr)
    {
        if (!other.IsValid())
            return;
        rawPtr = new T(args...);
        *rawPtr = *other.rawPtr;
    }

    Ptr(Ptr<T> &&other) : rawPtr(other.rawPtr)
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
        if (other.IsValid())
        {
            if (!rawPtr)
                rawPtr = new T;
            *rawPtr = *other.rawPtr;
        }
        else
        {
            rawPtr = nullptr;
        }
    }

    Ptr &operator=(Ptr<T> &&other)
    {
        if (IsValid())
            Release();
        rawPtr = other.rawPtr;
        other.rawPtr = 0;
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
};

template <class T> class WeakPtr;

template <class T> class SharedPtr
{
  protected:
    struct RefInfo
    {
        size_t counter;

        size_t weakCount;

        T *ptr;

        RefInfo(T *ptr) : counter(1), weakCount(0), ptr(ptr)
        {
        }
    };

    RefInfo *info;

    virtual void CleanUp()
    {
        if (!info)
            return;
        if (info->counter)
            info->counter--;
        if (!info->counter)
        {
            delete info->ptr;
            info->ptr = nullptr;
            if (!info->weakCount)
                delete info;
        }
        info = nullptr;
    }

    virtual void IncrementCounter()
    {
        if (info && info->counter)
            info->counter++;
    }

  public:
    using ValueType = T;

    template <class... Args> SharedPtr(const Args &...args)
    {
        info = new RefInfo(new T(args...));
    }

    SharedPtr(const SharedPtr<T> &other) : info(other.info)
    {
        IncrementCounter();
    }

    SharedPtr(const WeakPtr<T> &other) : info(other.info)
    {
        IncrementCounter();
    }

    SharedPtr(SharedPtr<T> &&other) : info(other.info)
    {
        other.info = nullptr;
    }

    SharedPtr(WeakPtr<T> &&other)
    {
        info = other.info;
        IncrementCounter();
        other.CleanUp();
    }

    SharedPtr(Ptr<T> &&other) : info(nullptr)
    {
        if (!other.IsValid())
            return;
        info = new RefInfo(other.GetRaw());
    }

    virtual ~SharedPtr()
    {
        CleanUp();
    }

    bool IsValid() const
    {
        return info && info->ptr;
    }

    T *GetRaw()
    {
        return IsValid() ? info->ptr : nullptr;
    }

    const T *GetRaw() const
    {
        return IsValid() ? info->ptr : nullptr;
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
        info = ref.info;
        IncrementCounter();
        return *this;
    }

    SharedPtr<T> &operator=(SharedPtr<T> &&ref)
    {
        CleanUp();
        info = ref.info;
        ref.info = nullptr;
        return *this;
    }

    bool operator==(const SharedPtr &other) const
    {
        return info == other.info;
    }

    bool operator==(const WeakPtr<T> &other) const
    {
        return info == other.info;
    }

    T &operator*()
    {
        return *info->ptr;
    }

    const T &operator*() const
    {
        return *info->ptr;
    }

    T *operator->()
    {
        return info->ptr;
    }

    const T *operator->() const
    {
        return info->ptr;
    }

    friend class WeakPtr<T>;
};

template <class T> class WeakPtr : public SharedPtr<T>
{
  private:
    void CleanUp() override
    {
        if (!this->info)
            return;
        if (this->info->weakCount)
            this->info->weakCount--;
        if (!this->info->counter && !this->info->weakCount)
            delete this->info;
        this->info = nullptr;
    }

    void IncrementCounter() override
    {
        if (this->info)
            this->info->weakCount++;
    }

  public:
    WeakPtr()
    {
        this->info = nullptr;
    }

    WeakPtr(const WeakPtr<T> &other)
    {
        this->info = other.info;
        IncrementCounter();
    }

    WeakPtr(WeakPtr<T> &&other)
    {
        this->info = other.info;
        other.info = nullptr;
    }

    WeakPtr(const SharedPtr<T> &other)
    {
        this->info = other.info;
        IncrementCounter();
    }

    WeakPtr(SharedPtr<T> &&other)
    {
        this->info = other.info;
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
        this->info = other.info;
        IncrementCounter();
        return *this;
    }

    WeakPtr<T> &operator=(WeakPtr<T> &&other)
    {
        CleanUp();
        this->info = other.info;
        other.info = 0;
        return *this;
    }

    T &operator*()
    {
        return *this->info->ptr;
    }

    const T &operator*() const
    {
        return *this->info->ptr;
    }

    T *operator->()
    {
        return this->info->ptr;
    }

    const T *operator->() const
    {
        return this->info->ptr;
    }

    friend class SharedPtr<T>;
};

#endif
