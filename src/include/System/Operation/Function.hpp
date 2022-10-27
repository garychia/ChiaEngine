#ifndef FUNCTION_HPP
#define FUNCTION_HPP

template <class T> class Function;

template <class T, class... Args> class Function<T(Args...)>
{
  private:
    typedef T (*FunctionType)(Args...);
    FunctionType pFunc;

  public:
    Function(T (*pFunc)(Args...) = nullptr) : pFunc(pFunc)
    {
    }

    T operator()(Args... args)
    {
        return (*pFunc)(args...);
    }

    bool Valid() const
    {
        return pFunc != nullptr;
    }
};

template <class ClassType, class T> class MemberFunction;

template <class ClassType, class T, class... Args> class MemberFunction<ClassType, T(Args...)>
{
  private:
    typedef T (ClassType::*FunctionType)(Args...);
    ClassType *pObject;
    FunctionType pFunc;

  public:
    MemberFunction(ClassType *pObject = nullptr, T (ClassType::*pFunc)(Args...) = nullptr)
        : pObject(pObject), pFunc(pFunc)
    {
    }

    T operator()(Args... args)
    {
        return (pObject->*pFunc)(args...);
    }

    bool Valid() const
    {
        return pObject && pFunc;
    }
};

#endif // FUNCTION_HPP
