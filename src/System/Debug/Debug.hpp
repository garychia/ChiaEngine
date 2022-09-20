#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <iostream>

#define PRINT_ERR(...) \
    Debug::PrintError("At File: ", __FILE__, "\nAt Line: ", __LINE__, "\n", __VA_ARGS__);

#define PRINTLN_ERR(...)                                                     \
    Debug::PrintError("At File: ", __FILE__, "\nAt Line: ", __LINE__, "\n"); \
    Debug::PrintLineError(__VA_ARGS__)

class Debug
{
public:
    Debug() = delete;

    template <class T>
    inline void Print(const T &arg)
    {
#ifdef DEBUG
        std::cout << arg;
#endif
    }

    template <class T, class U, class... Args>
    void Print(const T &arg1, const T &arg2, const Args &...args)
    {
        Print<T>(arg1);
        Print<U>(arg2);
        Print(args...);
    }

    template <class T>
    inline void PrintLine(const T &arg)
    {
#ifdef DEBUG
        std::cout << arg << std::endl;
#endif
    }

    template <class T, class U, class... Args>
    void PrintLine(const T &arg1, const T &arg2, const Args &...args)
    {
        PrintLine<T>(arg1);
        PrintLine<U>(arg2);
        PrintLine(args...);
    }

    template <class T>
    static inline void PrintError(const T &arg)
    {
#ifdef DEBUG
        std::cerr << arg;
#endif
    }

    template <class T, class U, class... Args>
    static void PrintError(const T &arg1, const U &arg2, const Args &...args)
    {
        PrintError<T>(arg1);
        PrintError<U>(arg2);
        PrintError(args...);
    }

    template <class T>
    static inline void PrintLineError(const T &arg)
    {
#ifdef DEBUG
        std::cerr << arg << std::endl;
#endif
    }

    template <class T, class U, class... Args>
    static void PrintLineError(const T &arg1, const U &arg2, const Args &...args)
    {
        PrintLineError<T>(arg1);
        PrintLineError<U>(arg2);
        PrintLineError(args...);
    }
};

#endif
