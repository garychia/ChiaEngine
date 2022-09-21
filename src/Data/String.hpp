#ifndef STRING_HPP
#define STRING_HPP

#include "Types/Types.hpp"
#include "Str.hpp"

class String : public Str<char16_t>
{
public:
    String() noexcept : Str<char16_t>() {}

    String(const Str<char16_t> &s) noexcept : Str<char16_t>(s) {}

    String(Str<char16_t> &&s) noexcept : Str<char16_t>(Move(s)) {}

    template <class T>
    String(const T &c, size_t length) noexcept : Str<char16_t>(c, length) {}

    template <class T>
    String(const T *str, size_t length) noexcept : Str<char16_t>(str, length) {}
};

class StringStream : public StrStream<String::CharType>
{
public:
    StringStream() : StrStream<String::CharType>() {}
};

#endif
