#ifndef STRING_HPP
#define STRING_HPP

#include "Str.hpp"
#include "Types/Types.hpp"

class String : public Str<char16_t>
{
  private:
    template <class Char>
    static size_t CalculateStrSize(const Char *cStr, size_t size)
    {
        size_t strLength = 0;
        for (size_t i = 0; i < size; i++)
        {
            strLength += cStr[i] < 0XD800 || cStr[i] > 0XDFFF ? 1 : 2;
        }
        return strLength;
    }

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

    template <class T> String(const T &c, size_t length) noexcept
    {
        const size_t strLength = c < 0XD800 || c > 0XDFFF ? length : length << 1;
        Str<char16_t>::Str(c, strLength);
        for (size_t i = 0; i < strLength; i++)
        {
            (*this)[i] = i % 2 == 0 ? ((c & 0XFFC00) >> 10) + 0XD800 : (c & 0X3FF) + 0XDC00;
        }
    }

    template <class T> String(const T *str, size_t length) noexcept : Str<CharType>('\0', CalculateStrSize<T>(str, length))
    {
        size_t currentIdx = 0;
        for (size_t i = 0; i < length; i++)
        {
            if (str[i] < 0XD800 || str[i] > 0XDFFF)
            {   
                (*this)[currentIdx] = str[i];
            }
            else
            {
                (*this)[currentIdx++] = ((str[i] & 0XFFC00) >> 10) + 0XD800;
                (*this)[currentIdx] = (str[i] & 0X3FF) + 0XDC00;
            }
            currentIdx++;
        }
    }

    template <class Char> String(const Char *cStr) : Str<char16_t>(cStr)
    {
        size_t strLength = 0;
        size_t i = 0;
        while (cStr[i])
        {
            strLength += cStr[i] < 0XD800 || cStr[i] > 0XDFFF ? 1 : 2;
            i++;
        }
        Str<char16_t>::Str('\0', strLength);
        const size_t cStrLength = i;
        size_t currentIdx = 0;
        for (i = 0; i < cStrLength; i++)
        {
            if (cStr[i] < 0XD800 || cStr[i] > 0XDFFF)
            {   
                (*this)[currentIdx] = cStr[i];
            }
            else
            {
                (*this)[currentIdx++] = ((cStr[i] & 0XFFC00) >> 10) + 0XD800;
                (*this)[currentIdx] = (cStr[i] & 0X3FF) + 0XDC00;
            }
            currentIdx++;
        }
    }

    Str<char> ToUTF8() const noexcept
    {
        size_t i = 0;
        size_t bufferIdx = 0;
        size_t targetLength = 0;
        while (i < Length())
        {
            if ((*this)[i] <= 0x7f)
                targetLength++;
            else if ((*this)[i] <= 0x7ff)
                targetLength += 2;
            else
                targetLength += 3;
            i++;
        }
        Str<char> result('\0', targetLength);
        size_t resultIdx = 0;
        i = 0;
        while (i < Length())
        {
            if ((*this)[i] <= 0x7f)
            {
                result[resultIdx++] = (*this)[i];
            }
            else if ((*this)[i] <= 0x7ff)
            {
                result[resultIdx++] = ((*this)[i] >> 6) | 0b11000000;
                result[resultIdx++] = 0b111111 & (*this)[i] | 0b10000000;
            }
            else
            {
                result[resultIdx++] = ((*this)[i] >> 12) | 0b11100000;
                result[resultIdx++] = 0b111111 & ((*this)[i] >> 6) | 0b10000000;
                result[resultIdx++] = 0b111111 & (*this)[i] | 0b10000000;
            }
            i++;
        }
        return result;
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
