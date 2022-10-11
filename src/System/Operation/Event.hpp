#ifndef EVENT_HPP
#define EVENT_HPP

#include "Data/HashTable.hpp"
#include "Function.hpp"
#include "Types/Types.hpp"

template <class T> class Callback;

template <class T, class... Args> class Callback<T(Args...)>
{
  private:
    class IWrapper
    {
      public:
        virtual T Invoke(Args... args) = 0;
    };

    template <class Subscriber> class MemberFunctionWrapper : public IWrapper
    {
      private:
        MemberFunction<Subscriber, T(Args...)> func;

      public:
        MemberFunctionWrapper(Subscriber *pObect, T (Subscriber::*pFunc)(Args...)) : func(pObect, pFunc)
        {
        }

        virtual T Invoke(Args... args) override
        {
            return func(args...);
        }
    };

    class FunctionWrapper : public IWrapper
    {
      private:
        Function<T(Args...)> func;

      public:
        FunctionWrapper(T (*pFunc)(Args...)) : func(pFunc)
        {
        }

        virtual T Invoke(Args... args) override
        {
            return func(args...);
        }
    };

    IWrapper *pCallback;

  public:
    Callback() : pCallback(nullptr)
    {
    }

    Callback(const Callback &) = delete;

    Callback &operator=(const Callback &) = delete;

    template <class Subscriber> Callback(Subscriber *pObject = nullptr, T (Subscriber::*pFunc)(Args...) = nullptr)
    {
        pCallback = pObject && pFunc ? new MemberFunctionWrapper<Subscriber>(pObject, pFunc) : nullptr;
    }

    ~Callback()
    {
        if (pCallback)
            delete pCallback;
    }

    template <class Subscriber> void Set(Subscriber *pObject, T (Subscriber::*pFunc)(Args...))
    {
        if (pCallback)
            delete pCallback;
        pCallback = pObject && pFunc ? new MemberFunctionWrapper<Subscriber>(pObject, pFunc) : nullptr;
    }

    void Set(T (*pFunc)(Args...))
    {
        if (pCallback)
            delete pCallback;
        pCallback = pFunc ? new FunctionWrapper(pFunc) : nullptr;
    }

    T operator()(Args... args)
    {
        return pCallback->Invoke(args...);
    }

    bool Valid() const
    {
        return pCallback;
    }
};

template <class T> class Event;

template <class T, class... Args> class Event<T(Args...)>
{
  private:
    HashTable<void *, Callback<T(Args...)> *> callbacks;

  public:
    Event() : callbacks()
    {
    }

    Event(const Event &other) : callbacks(other.callbacks)
    {
    }

    Event(Event &&other) : callbacks(Move(other.callbacks))
    {
    }

    Event &operator=(const Event &other)
    {
        callbacks = other.callbacks;
    }

    Event &operator=(Event &&other)
    {
        callbacks = Types::Move(other.callbacks);
    }

    ~Event()
    {
        Clear();
    }

    template <class Subscriber> void Subscribe(Subscriber *pSubstruber, T (Subscriber::*pFunc)(Args...))
    {
        callbacks.Insert((void *)pSubstruber, new Callback<T(Args...)>(pSubstruber, pFunc));
    }

    template <class Subscriber> void Unsubscribe(Subscriber *pSubstruber)
    {
        callbacks.Remove((void *)pSubstruber);
    }

    void Invoke(Args... args)
    {
        for (auto itr = callbacks.First(); itr != callbacks.Last(); itr++)
        {
            (*itr->Value())(args...);
        }
    }

    void Clear()
    {
        callbacks.Clear();
    }
};

#endif // EVENT_HPP
