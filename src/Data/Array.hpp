#ifndef ARRAY_HPP
#define ARRAY_HPP

#include <initializer_list>

template <class T> class Array
{
  protected:
    T *data = 0;
    size_t length = 0;

    inline void ReleaseData()
    {
        if (data)
            delete[] data;
        data = 0;
        length = 0;
    }

    inline void AllocateData(size_t size)
    {
        length = size;
        data = length ? new T[length]{} : 0;
    }

    inline void CopyData(const T *newData, size_t size)
    {
        for (size_t i = 0; i < size; i++)
            data[i] = newData[i];
    }

    inline void CopyData(const T &element, size_t size)
    {
        for (size_t i = 0; i < size; i++)
            data[i] = element;
    }

  public:
    using ValueType = T;

    Array() noexcept
    {
    }

    virtual ~Array() noexcept
    {
        ReleaseData();
    }

    Array(size_t initialSize) noexcept
    {
        AllocateData(initialSize);
    }

    Array(const Array &arr) noexcept
    {
        AllocateData(arr.Length());
        CopyData(arr.data, arr.Length());
    }

    Array(const std::initializer_list<T> &initList)
    {
        AllocateData(initList.size());
        size_t idx = 0;
        for (const auto &element : initList)
            data[idx++] = element;
    }

    Array(Array &&arr) noexcept
    {
        length = arr.Length();
        data = arr.data;
        arr.length = 0;
        arr.data = 0;
    }

    Array(const T &element, size_t nElements) noexcept
    {
        AllocateData(nElements);
        CopyData(element, nElements);
    }

    Array(const T *cArr, size_t nElements) noexcept
    {
        AllocateData(nElements);
        CopyData(cArr, nElements);
    }

    Array<T> &operator=(const Array<T> &arr) noexcept
    {
        ReleaseData();
        AllocateData(arr.Length());
        CopyData(arr.data, arr.Length());
        return *this;
    }

    Array<T> &operator=(Array<T> &&arr) noexcept
    {
        ReleaseData();
        length = arr.Length();
        data = arr.data;
        arr.length = 0;
        arr.data = 0;
        return *this;
    }

    template <class Comparator> bool operator==(const Array<T> &other) const noexcept
    {
        Comparator cmp;
        if (length != other.Length())
            return false;
        for (size_t i = 0; i < length; i++)
        {
            if (!cmp((*this)[i], other[i]))
                return false;
        }
        return true;
    }

    inline T *operator*() noexcept
    {
        return data;
    }

    inline const T *operator*() const noexcept
    {
        return data;
    }

    inline T &operator[](size_t index) noexcept
    {
        return data[index];
    }

    inline const T &operator[](size_t index) const noexcept
    {
        return data[index];
    }

    inline virtual size_t Length() const noexcept
    {
        return length;
    }

    inline virtual T &GetFirst() noexcept
    {
        return data[0];
    }

    inline virtual const T &GetFirst() const noexcept
    {
        return data[0];
    }

    inline virtual T &GetLast() noexcept
    {
        return data[length - 1];
    }

    inline virtual const T &GetLast() const noexcept
    {
        return data[length - 1];
    }
};

#endif
