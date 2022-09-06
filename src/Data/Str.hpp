#ifndef STR_HPP
#define STR_HPP

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
        if (length > 0)
        {
            ptr = new T[length];
            for (size_t i = 0; i < length; i++)
                ptr[i] = ptrToCopy[i];
        }
        else
        {
            ptr = 0;
        }
    }

public:
    using CharType = T;

    template <class Int>
    static Str<T> FromInt(const Int &i)
    {
        List<T> digits;
        Int n = i;
        bool negative = n < 0;
        do
        {
            digits.Prepend('0' + -(n % 10));
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

    Str() noexcept {}

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
        for (size_t i = 0; N && i < N - 1; i++)
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
        if (length)
        {
            ptr = new T[length];
            for (size_t i = 0; i < length; i++)
                ptr[i] = (T)c;
        }
        else
        {
            ptr = 0;
        }
    }

    template <class U>
    Str(const U *str, size_t length) noexcept
    {
        CopyPtr<U>(str, length);
    }

    size_t Length() const noexcept { return length; }

    Str<T> SubStr(size_t start, long long end = -1) const noexcept
    {
        if (-1 == end)
            end = length - 1;
        if (start > end)
            return Str<T>();
        return Str<T>(&ptr[start], end - start + 1);
    }

    Str<T> &Shrink(size_t newSize) noexcept
    {
        if (newSize >= length)
            return *this;
        length = newSize;
        return *this;
    }

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
        strs.Append(std::forward<StrType>(str));
    }

    template <class Int>
    StrStream &operator<<(const Int &i) noexcept
    {
        strs.Append(Str<T>::FromInt(i));
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