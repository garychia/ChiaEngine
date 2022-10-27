#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <iostream>

#define PRINT_ERR(...) Debug::PrintError("At File: ", __FILE__, "\nAt Line: ", __LINE__, "\n", __VA_ARGS__)

#define PRINTLN_ERR(...)                                                                                               \
    Debug::PrintError("At File: ", __FILE__, "\nAt Line: ", __LINE__, "\n");                                           \
    Debug::PrintLineError(__VA_ARGS__)

class Debug
{
  public:
    Debug() = delete;

    static void Print()
    {
    }

    template <class T> static void Print(const T &arg)
    {
#ifdef DEBUG
        std::cout << arg;
#endif
    }

    template <class T, class U, class... Args> static void Print(const T &arg1, const U &arg2, const Args &...args)
    {
#ifdef DEBUG
        std::cout << arg1;
        std::cout << arg2;
        Print(args...);
#endif
    }

    static void PrintLine()
    {
        std::cout << std::endl;
    }

    template <class T> static void PrintLine(const T &arg)
    {
#ifdef DEBUG
        std::cout << arg << std::endl;
#endif
    }

    template <class T, class U, class... Args> static void PrintLine(const T &arg1, const U &arg2, const Args &...args)
    {
#ifdef DEBUG
        std::cout << arg1 << std::endl;
        std::cout << arg2 << std::endl;
        PrintLine(args...);
#endif
    }

    static void PrintError()
    {
    }

    template <class T> static void PrintError(const T &arg)
    {
#ifdef DEBUG
        std::cerr << arg;
#endif
    }

    template <class T, class U, class... Args> static void PrintError(const T &arg1, const U &arg2, const Args &...args)
    {
#ifdef DEBUG
        std::cerr << arg1;
        std::cerr << arg2;
        PrintError(args...);
#endif
    }

    static void PrintLineError()
    {
        std::cerr << std::endl;
    }

    template <class T> static void PrintLineError(const T &arg)
    {
#ifdef DEBUG
        std::cerr << arg << std::endl;
#endif
    }

    template <class T, class U, class... Args>
    static void PrintLineError(const T &arg1, const U &arg2, const Args &...args)
    {
#ifdef DEBUG
        std::cerr << arg1 << std::endl;
        std::cerr << arg2 << std::endl;
        PrintLineError(args...);
#endif
    }
};

#endif
