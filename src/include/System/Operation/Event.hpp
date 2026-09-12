#ifndef EVENT_HPP
#define EVENT_HPP

#include "Data/DynamicArray.hpp"
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
        virtual ~IWrapper() = default;
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
    // Subscription-ordered dispatch (issue #79): subscribers and callbacks are
    // held as PARALLEL DynamicArrays in subscribe order. Invoke iterates in
    // subscription order (deterministic, replay-stable), and Event OWNS every
    // Callback* — Unsubscribe/Clear/dtors delete them (no leaks).
    DynamicArray<void *> subscribers;
    DynamicArray<Callback<T(Args...)> *> callbacks;

  public:
    Event() : subscribers(), callbacks()
    {
    }

    Event(const Event &other) = delete;

    Event &operator=(const Event &other) = delete;

    Event(Event &&other) = delete;

    Event &operator=(Event &&other) = delete;

    ~Event()
    {
        Clear();
    }

    template <class Subscriber> void Subscribe(Subscriber *pSubscriber, T (Subscriber::*pFunc)(Args...))
    {
        Callback<T(Args...)> *pCallback = new Callback<T(Args...)>(pSubscriber, pFunc);
        // Duplicate subscribe on the same subscriber: replace in place (no leak).
        for (size_t i = 0; i < subscribers.Length(); i++)
        {
            if (subscribers[i] == (void *)pSubscriber)
            {
                delete callbacks[i];
                callbacks[i] = pCallback;
                return;
            }
        }
        subscribers.Append((void *)pSubscriber);
        callbacks.Append(pCallback);
    }

    template <class Subscriber> void Unsubscribe(Subscriber *pSubscriber)
    {
        for (size_t i = 0; i < subscribers.Length(); i++)
        {
            if (subscribers[i] == (void *)pSubscriber)
            {
                delete callbacks[i];
                // Swap-with-last removal keeps the array dense; remaining
                // dispatch order stays the ORIGINAL subscribe order.
                callbacks[i] = callbacks[callbacks.Length() - 1];
                subscribers[i] = subscribers[subscribers.Length() - 1];
                callbacks.RemoveLast();
                subscribers.RemoveLast();
                return;
            }
        }
    }

    void Invoke(Args... args)
    {
        for (size_t i = 0; i < callbacks.Length(); i++)
        {
            (*callbacks[i])(args...);
        }
    }

    void Clear()
    {
        for (size_t i = 0; i < callbacks.Length(); i++)
        {
            delete callbacks[i];
            callbacks[i] = nullptr;
        }
        callbacks.RemoveAll();
        subscribers.RemoveAll();
    }

    size_t Length() const
    {
        return callbacks.Length();
    }
};

#endif // EVENT_HPP