#ifndef DYNAMIC_ARRAY_HPP
#define DYNAMIC_ARRAY_HPP

#include "Array.hpp"
#include "Types/Types.hpp"

template <class T> class DynamicArray : public Array<T>
{
  private:
    size_t nElements;

    inline void ResizeArray(size_t newSize) noexcept
    {
        auto oldData = this->data;
        this->AllocateData(newSize);
        for (size_t i = 0; i < nElements && i < newSize; i++)
            this->data[i] = oldData[i];
        if (oldData)
            delete[] oldData;
    }

  public:
    DynamicArray() noexcept : Array<T>(), nElements(0)
    {
    }

    DynamicArray(const std::initializer_list<T> &initList) noexcept : Array<T>(initList), nElements(initList.size())
    {
    }

    DynamicArray(const Array<T> &arr) noexcept : Array<T>(arr)
    {
        nElements = arr.GetNElements();
    }

    DynamicArray(Array<T> &&arr) noexcept
    {
        nElements = arr.GetNElements();
        arr.ResetNElements();
        Array<T>::Array(Types::Forward<Array<T>>(arr));
    }

    DynamicArray(size_t initialSize) noexcept : Array<T>(initialSize), nElements(initialSize)
    {
    }

    DynamicArray(const T &element, size_t nElements) noexcept : Array<T>(element, nElements), nElements(nElements)
    {
    }

    DynamicArray(const T *cArr, size_t nElements) noexcept : Array<T>(cArr, nElements), nElements(nElements)
    {
    }

    DynamicArray<T> &operator=(const std::initializer_list<T> &l)
    {
        *this = DynamicArray<T>(l);
        return *this;
    }

    DynamicArray<T> &operator=(const Array<T> &arr) noexcept
    {
        Array<T>::operator=(arr);
        nElements = arr.GetNElements();
        return *this;
    }

    DynamicArray<T> &operator=(Array<T> &&arr) noexcept
    {
        nElements = arr.GetNElements();
        arr.ResetNElements();
        Array<T>::operator=(Move(arr));
        return *this;
    }

    template <class Comparator> bool operator==(const Array<T> &other) const noexcept
    {
        Comparator cmp;
        size_t otherNElements = other.GetNElements();
        if (nElements != otherNElements)
            return false;
        for (size_t i = 0; i < nElements; i++)
        {
            if (!cmp((*this)[i], other[i]))
                return false;
        }
        return true;
    }

    template <class Element> inline void Append(Element &&e) noexcept
    {
        // Grow-only on append (issue #85): never shrink here — the buffer is a
        // reusable resource across frames (Frame::Clear keeps capacity now).
        if (nElements >= this->length)
            ResizeArray(this->length > 2 ? this->length << 1 : 4);
        this->data[nElements++] = Types::Forward<decltype(e)>(e);
    }

    inline void RemoveLast() noexcept
    {
        if (this->IsEmpty())
            return;
        this->data[--nElements] = T();
        // Exponential shrink headroom (issue #85): shrink only when below 1/4
        // capacity, and never below 4 elements — matches std::vector amortized
        // behavior, so per-removal churn (ComponentPool/EntityRegistry destroy
        // paths) stops being O(n^2)-ish.
        if ((this->length > 8) && nElements < (this->length >> 2))
            ResizeArray(this->length >> 1);
    }

    inline void RemoveAll() noexcept
    {
        // Issue #85: KEEP capacity — callers do not free, they reset. Frame::Clear()
        // runs every frame; dropping the buffer here means a cold realloc every frame.
        nElements = 0;
    }

    inline void Resize(size_t newSize) noexcept
    {
        ResizeArray(newSize);
        nElements = newSize;
    }

    inline bool IsEmpty() const noexcept
    {
        return nElements == 0;
    }

    inline virtual size_t Length() const noexcept override
    {
        return nElements;
    }

    inline virtual T &GetFirst() noexcept override
    {
        assert(!IsEmpty() && "DynamicArray::GetFirst on empty container");
        return this->data[0];
    }

    inline virtual const T &GetFirst() const noexcept override
    {
        assert(!IsEmpty() && "DynamicArray::GetFirst on empty container");
        return this->data[0];
    }

    inline virtual T &GetLast() noexcept override
    {
        assert(!IsEmpty() && "DynamicArray::GetLast on empty container");
        return this->data[nElements - 1];
    }

    inline virtual const T &GetLast() const noexcept override
    {
        assert(!IsEmpty() && "DynamicArray::GetLast on empty container");
        return this->data[nElements - 1];
    }

    size_t GetNElements() const noexcept override { return nElements; }
    void ResetNElements() noexcept override { nElements = 0; }
};

#endif
