#ifndef STR_HPP
#define STR_HPP

#include "Types/Types.hpp"
#include "List.hpp"

template <class T>
class Str
{
private:
    T *ptr = 0;
    size_t length = 0;

    template <class U>
    void CopyPtr(const U *ptrToCopy, size_t len) noexcept
    {
        length = len;
        ptr = new T[length + 1];
        for (size_t i = 0; i < length; i++)
            ptr[i] = ptrToCopy[i];
        ptr[length] = '\0';
    }

public:
    using CharType = T;

    template <class Int>
    static Str<T> FromInt(const Int &i) noexcept
    {
        List<T> digits;
        Int n = i;
        bool negative = n < 0;
        do
        {
            digits.Prepend('0' + (negative ? -(n % 10) : n % 10));
            n /= 10;
        } while (n);
        if (negative)
            digits.Prepend('-');
        Str<T> result(' ', digits.Length());
        size_t idx = 0;
        for (auto current = &digits.First(); current; current = &current->GetNext())
            result[idx++] = current->GetData();
        return result;
    }

    template <class FloatType>
    static Str<T> FromFloat(const FloatType &f, size_t precision = 3)
    {
        List<T> digits;
        bool negative = f < 0;
        long long intPart = f;
        FloatType decimalPart = f - intPart;
        do
        {
            digits.Prepend('0' + (negative ? -(intPart % 10) : intPart % 10));
            intPart /= 10;
        } while (intPart);
        if (negative)
            digits.Prepend('-');
        if (precision)
            digits.Append('.');
        while (precision > 0)
        {
            decimalPart *= 10;
            const auto digit = negative ? -((long long)decimalPart % 10) : (long long)decimalPart % 10;
            digits.Append('0' + digit);
            precision--;
        }
        Str<T> result(' ', digits.Length());
        size_t idx = 0;
        for (auto current = &digits.First(); current; current = &current->GetNext())
            result[idx++] = current->GetData();
        return result;
    }

    Str() noexcept : length(0), ptr(new T[1])
    {
        ptr[0] = '\0';
    }

    Str(const Str<T> &s) noexcept
    {
        CopyPtr<T>(s.ptr, s.length);
    }

    Str(Str<T> &&s) noexcept
    {
        length = s.length;
        ptr = s.ptr;
        s.length = 0;
        s.ptr = 0;
    }

    Str<T> &operator=(const Str<T> &s) noexcept
    {
        CopyPtr(&s[0], s.length);
        return *this;
    }

    Str<T> &operator=(Str<T> &&s) noexcept
    {
        length = s.length;
        if (ptr)
            delete[] ptr;
        ptr = s.ptr;
        s.length = 0;
        s.ptr = 0;
        return *this;
    }

    bool operator==(const Str<T> &s) const noexcept
    {
        if (length != s.length)
            return false;
        for (size_t i = 0; i < length; i++)
        {
            if ((*this)[i] != s[i])
                return false;
        }
        return true;
    }

    template <class Char, size_t N>
    bool operator==(Char (&s)[N]) const noexcept
    {
        if (length + 1 != N)
            return false;
        for (size_t i = 0; i < length; i++)
        {
            if ((*this)[i] != s[i])
                return false;
        }
        return true;
    }

    template <class Char, size_t N>
    bool operator!=(Char (&s)[N]) const noexcept
    {
        return !operator==(s);
    }

    Str(const T &c, size_t length = 1) noexcept
    {
        this->length = length;
        ptr = new T[length + 1];
        for (size_t i = 0; i < length; i++)
            ptr[i] = (T)c;
        ptr[length] = '\0';
    }

    template <class U>
    Str(const U *str, size_t length) noexcept
    {
        CopyPtr<U>(str, length);
    }

    size_t Length() const noexcept { return length; }

    Str<T> SubStr(size_t start) const noexcept
    {
        return Str<T>(&ptr[start], length - start);
    }

    Str<T> SubStr(size_t start, size_t end) const noexcept
    {
        if (start > end)
            return Str<T>();
        if (end + 1 > length)
            return SubStr(start);
        return Str<T>(&ptr[start], end - start + 1);
    }

    Str<T> &Shrink(size_t newSize) noexcept
    {
        if (newSize >= length)
            return *this;
        length = newSize;
        return *this;
    }

    T *CStr() noexcept { return ptr; }

    const T *CStr() const noexcept { return ptr; }

    T &operator[](size_t index) noexcept
    {
        return ptr[index];
    }

    const T &operator[](size_t index) const noexcept
    {
        return ptr[index];
    }

    Str<T> operator+(const Str<T> &s) const noexcept
    {
        Str<T> result(' ', length + s.length);
        size_t idx = 0;
        for (size_t i = 0; i < length; i++)
            result[idx++] = (*this)[i];
        for (size_t i = 0; i < s.length; i++)
            result[idx++] = s[i];
        return result;
    }

    virtual ~Str() noexcept
    {
        if (ptr)
            delete[] ptr;
    }

    template <class T>
    long long Find(const T &c) noexcept
    {
        for (size_t i = 0; i < length; i++)
        {
            if (ptr[i] == c)
                return i;
        }
        return -1;
    }
};

template <class T>
class StrStream
{
private:
    List<Str<T>> strs;

public:
    using StrType = Str<T>;

    StrStream() : strs() {}

    template <class StrType>
    StrStream &operator<<(StrType &&str) noexcept
    {
        strs.Append(Forward<StrType>(str));
    }

    template <class T>
    StrStream &operator<<(const T &input) noexcept
    {
        if (IsChar<T>::Value)
            strs.Append(Str<T>(input));
        else if (IsInteger<T>::Value)
            strs.Append(Str<T>::FromInt(input));
        else
            strs.Append(Str<T>::FromFloat(input));
    }

    Str<T> ToString() const noexcept
    {
        size_t length = 0;
        for (auto current = &strs.First(); current; current = &current->GetNext())
        {
            const List<Str<T>>::ListElement &e = *current;
            length += (*current)->Length();
        }
        size_t idx = 0;
        Str<T> result(' ', length);
        for (auto current = &strs.First(); current; current = &current->GetNext())
        {
            const List<Str<T>>::ListElement &e = *current;
            for (size_t i = 0; i < e->Length(); i++)
                result[idx++] = (*e)[i];
        }
        return result;
    }
};

#endif
