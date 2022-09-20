#ifndef STRING_HPP
#define STRING_HPP

#define STR(x) String((const wchar_t *)(L##x), (size_t)(sizeof(L##x) / sizeof(wchar_t) - 1))
#define CHAR char16_t

#include "Types/Types.hpp"
#include "Str.hpp"

class String : public Str<CHAR>
{
public:
    String() noexcept : Str<CHAR>() {}

    String(const Str<CHAR> &s) noexcept : Str<CHAR>(s) {}

    String(Str<CHAR> &&s) noexcept : Str<CHAR>(Move(s)) {}

    template <class T>
    String(const T &c, size_t length) noexcept : Str<CHAR>(c, length) {}

    template <class T>
    String(const T *str, size_t length) noexcept : Str<CHAR>(str, length) {}
};

class StringStream : StrStream<String::CharType>
{
public:
    StringStream() : StrStream<String::CharType>() {}
};

#endif
