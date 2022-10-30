#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <iostream>

#include "Data/String.hpp"

#ifndef NDEBUG
#define PRINT_ERR(msg)                                                                                                 \
    Debug::Print(L"At File: ", String(__FILE__), L"\nAt Line: ", String(__LINE__), L"\n", String(msg))
#define PRINTLN_ERR(msg)                                                                                               \
    Debug::Print(L"At File: ", String(__FILE__), L"\nAt Line: ", String(__LINE__), L"\n");                             \
    Debug::PrintLine(String(msg))
#else
#define PRINT_ERR(msg)
#define PRINTLN_ERR(msg)
#endif

class Debug
{
  public:
    static void Print(const String &msg);

    static void Print(const wchar_t *msg);

    template <class T> static void Print(const T &msg)
    {
        Print(msg);
    }

    template <class T, class U, class... Strings> static void Print(const T &msg1, const U &msg2, const Strings &...args)
    {
        Print(msg1);
        Print(msg2);
        Print<Strings...>(args...);
    }

    static void PrintLine(const String &msg);

    static void PrintLine(const wchar_t *msg);

    template <class T> static void PrintLine(const T &msg)
    {
        PrintLine(msg);
    }

    template <class T, class U, class... Strings> static void PrintLine(const T &msg1, const U &msg2, const Strings &...args)
    {
        PrintLine(msg1);
        PrintLine(msg2);
        PrintLine<Strings...>(args...);
    }
};

#endif
