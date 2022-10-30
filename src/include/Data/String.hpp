#ifndef STRING_HPP
#define STRING_HPP

#include "Str.hpp"
#include "Types/Types.hpp"

class String : public Str<char16_t>
{
  public:
    String() noexcept : Str<char16_t>()
    {
    }

    String(const Str<char16_t> &s) noexcept : Str<char16_t>(s)
    {
    }

    String(Str<char16_t> &&s) noexcept : Str<char16_t>(Types::Move(s))
    {
    }

    template <class T> String(const T &c, size_t length) noexcept : Str<char16_t>(c, length)
    {
    }

    template <class T> String(const T *str, size_t length) noexcept : Str<char16_t>(str, length)
    {
    }

    template <class Char> String(const Char *cStr) : Str<char16_t>(cStr)
    {
    }

    size_t ToUTF8(char *buffer, size_t bufferSize) const noexcept
    {
        if (!buffer || !bufferSize)
            return 0;
        size_t i = 0;
        size_t bufferIdx = 0;
        while (i < Length() && bufferIdx < bufferSize - 1)
        {
            if ((*this)[i] <= 0x7f)
            {
                buffer[bufferIdx++] = (*this)[i];
            }
            else if ((*this)[i] <= 0x7ff)
            {
                buffer[bufferIdx++] = ((*this)[i] >> 6) | 0b11000000;
                if (bufferIdx < bufferSize - 1)
                    buffer[bufferIdx++] = 0b111111 & (*this)[i] | 0b10000000;
            }
            else
            {
                buffer[bufferIdx++] = ((*this)[i] >> 12) | 0b11100000;
                if (bufferIdx < bufferSize - 1)
                    buffer[bufferIdx++] = 0b111111 & ((*this)[i] >> 6) | 0b10000000;
                if (bufferIdx < bufferSize - 1)
                    buffer[bufferIdx++] = 0b111111 & (*this)[i] | 0b10000000;
            }
            i++;
        }
        buffer[bufferIdx] = '\0';
        return bufferIdx;
    }
};

class StringStream : public StrStream<String::CharType>
{
  public:
    StringStream() : StrStream<String::CharType>()
    {
    }
};

#endif
