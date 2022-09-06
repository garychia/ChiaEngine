#ifndef STRING_HPP
#define STRING_HPP

#define STR(x) String((const wchar_t *)(L##x), (size_t)(sizeof(L##x) / sizeof(wchar_t)))
#define CHAR wchar_t

#include <tchar.h>
#include <utility>

#include "Str.hpp"

class String : public Str<wchar_t>
{
public:
    String() noexcept : Str<wchar_t>() {}

    String(const Str<wchar_t> &s) noexcept : Str<wchar_t>(s) {}

    String(Str<wchar_t> &&s) noexcept : Str<wchar_t>(std::forward<Str<wchar_t>>(s)) {}

    template <class T>
    String(const T &c, size_t length) noexcept : Str<wchar_t>(c, length) {}

    template <class T>
    String(const T *str, size_t length) noexcept : Str<wchar_t>(str, length) {}
};

class StringStream : StrStream<String::CharType>
{
public:
    StringStream() : StrStream<String::CharType>() {}
};

#endif