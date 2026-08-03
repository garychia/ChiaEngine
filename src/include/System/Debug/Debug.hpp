#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <iostream>

#include "Data/String.hpp"

#ifndef NDEBUG
#define PRINT_ERR(msg)                                                                                                 \
    Debug::Print(L"At File: ", String(__FILE__), L"\nAt Line: ", Str<char16_t>::FromInt(__LINE__), L"\n", String(msg))
#define PRINTLN_ERR(msg)                                                                                               \
    Debug::Print(L"At File: ", String(__FILE__), L"\nAt Line: ", Str<char16_t>::FromInt(__LINE__), L"\n");             \
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
        // fallback:轉成 String 走非模板 overload。
        // 直接 Print(msg) 會讓 T=Str<char16_t> 時無限自遞迴(String 是它的子類,
        // 非模板 Print(const String&) 接不住 base ref,模板自己勝出)。
        Print(String(msg));
    }

    template <class T, class... Strings> static void Print(const T &msg, const Strings &...args)
    {
        Print(msg);
        Print<Strings...>(args...);
    }

    static void PrintLine(const String &msg);

    static void PrintLine(const wchar_t *msg);

    template <class T> static void PrintLine(const T &msg)
    {
        PrintLine(String(msg));
    }

    template <class T, class... Strings> static void PrintLine(const T &msg, const Strings &...args)
    {
        PrintLine(msg);
        PrintLine<Strings...>(args...);
    }
};

#endif
