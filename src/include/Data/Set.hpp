#ifndef SET_HPP
#define SET_HPP

#include "Types/Types.hpp"
#include "Data/String.hpp"

#include <initializer_list>

template <class T> class Set
{
  private:
    class ValueHash
    {
      public:
        template <class U> static size_t Generate(const U &value)
        {
            return ((size_t)value & 0xff) * 17 + (((size_t)value >> 8) & 0xff) * 13 +
                   (((size_t)value >> 16) & 0xff) * 3;
        }

        template <> static size_t Generate(const String &s)
        {
            size_t hashValue = 0;
            for (size_t i = 0; i < s.Length(); i++)
                hashValue += s[i] * i * 7;
            return hashValue;
        }

        template <class Char> static size_t Generate(const Str<Char> &s)
        {
            size_t hashValue = 0;
            for (size_t i = 0; i < s.Length(); i++)
                hashValue += s[i] * i * 7;
            return hashValue;
        }
    };

    class ProbeHash
    {
      public:
        template <class U> static size_t Generate(const U &value)
        {
            return ((size_t)value & 0xff) * 7 + (((size_t)value >> 8) & 0xff) * 11 +
                   (((size_t)value >> 16) & 0xff) * 19;
        }

        template <> static size_t Generate(const String &s)
        {
            size_t hashValue = 0;
            for (size_t i = 0; i < s.Length(); i++)
                hashValue += s[i] * i * 13;
            return hashValue;
        }

        template <class Char> static size_t Generate(const Str<Char> &s)
        {
            size_t hashValue = 0;
            for (size_t i = 0; i < s.Length(); i++)
                hashValue += s[i] * i * 13;
            return hashValue;
        }
    };

    static const size_t SetSizes[];

    size_t size;

    size_t sizeIndex;

    size_t nElements;

    T *data;

    bool *insertMarks;

    bool *deleteMarks;

    void AllocateMemory()
    {
        data = new T[size]{};
        insertMarks = new bool[size];
        deleteMarks = new bool[size];
        for (size_t i = 0; i < size; i++)
            insertMarks[i] = deleteMarks[i] = false;
    }

    void ReleaseMemory()
    {
        if (data)
            delete[] data;
        if (insertMarks)
            delete[] insertMarks;
        if (deleteMarks)
            delete[] deleteMarks;
    }

    void DynamicallyResize()
    {
        const float loadFactor = (float)nElements / size;
        if (loadFactor >= 0.75f)
            Expand();
        else if (loadFactor <= 0.3f)
            Shrink();
    }

    void Expand()
    {
        size_t oldSize = size;
        auto oldData = data;
        auto oldInsertMarks = insertMarks;
        auto oldDeleteMarks = deleteMarks;
        if (sizeIndex < sizeof(SetSizes) / sizeof(size_t))
        {
            sizeIndex++;
            size = SetSizes[sizeIndex];
            AllocateMemory();
        }
        else
        {
            size <<= 1;
            AllocateMemory();
        }
        for (size_t i = 0; i < oldSize; i++)
        {
            if (!oldInsertMarks[i] || oldDeleteMarks[i])
                continue;
            Insert(oldData[i]);
        }
        delete[] oldData;
        delete[] oldInsertMarks;
        delete[] oldDeleteMarks;
    }

    void Shrink()
    {
        if (sizeIndex == 0)
            return;
        size_t oldSize = size;
        auto oldData = data;
        auto oldInsertMarks = insertMarks;
        auto oldDeleteMarks = deleteMarks;
        if (sizeIndex < sizeof(SetSizes) / sizeof(size_t))
        {
            sizeIndex--;
            size = SetSizes[sizeIndex];
            AllocateMemory();
        }
        else if ((size >> 1) == SetSizes[sizeof(SetSizes) / sizeof(size_t) - 1])
        {
            sizeIndex = sizeof(SetSizes) / sizeof(size_t) - 1;
            size = SetSizes[sizeIndex];
            AllocateMemory();
        }
        else
        {
            size >>= 1;
            AllocateMemory();
        }
        for (size_t i = 0; i < oldSize; i++)
        {
            if (!oldInsertMarks[i] || oldDeleteMarks[i])
                continue;
            Insert(oldData[i]);
        }
        delete[] oldData;
        delete[] oldInsertMarks;
        delete[] oldDeleteMarks;
    }

    size_t FindPosition(const T &value, bool search) const
    {
        const auto hashValue = ValueHash::Generate(value);
        auto currentIdx = hashValue % size;
        if (insertMarks[currentIdx] && !deleteMarks[currentIdx] && data[currentIdx] == value)
            return currentIdx;
        if (!search && (!insertMarks[currentIdx] || deleteMarks[currentIdx]))
            return currentIdx;

        size_t i = 1;
        const auto stepSize = ProbeHash::Generate(value) % (size - 1) + 1;
        currentIdx = (hashValue + i * stepSize) % size;
        while (insertMarks[currentIdx])
        {
            if (deleteMarks[currentIdx] && !search)
                break;
            else if (!deleteMarks[currentIdx] && data[currentIdx] == value)
                break;
            i++;
            currentIdx = (hashValue + i * stepSize) % size;
        }
        return currentIdx;
    }

  public:
    class Iterator
    {
      private:
        Set<T> *owner;
        size_t idx;
        size_t startIdx;

        void FindNext()
        {
            if (!owner)
                return;
            if (idx < owner->size)
                idx++;
            while (idx < owner->size && (!owner->insertMarks[idx] || owner->deleteMarks[idx]))
                idx++;
        }

        void FindPrev()
        {
            if (!owner)
                return;
            if (idx > startIdx)
                idx--;
            while (idx > startIdx && (!owner->insertMarks[idx] || owner->deleteMarks[idx]))
                idx--;
        }

      public:
        Iterator(Set *pSet = nullptr, bool end = false) : owner(pSet), idx(0), startIdx(0)
        {
            if (!pSet)
                return;
            if (end)
            {
                idx = pSet->size;
                return;
            }
            while (idx < pSet->size && (!pSet->insertMarks[idx] || pSet->deleteMarks[idx]))
                idx++;
            startIdx = idx;
        }

        T &operator*()
        {
            return owner->data[idx];
        }

        T *operator->()
        {
            return &owner->data[idx];
        }

        Set<T>::Iterator &operator++()
        {
            FindNext();
            return *this;
        }

        Set<T>::Iterator operator++(int)
        {
            FindNext();
            return *this;
        }

        Set<T>::Iterator &operator--()
        {
            FindPrev();
            return *this;
        }

        Set<T>::Iterator operator--(int)
        {
            FindPrev();
            return *this;
        }

        bool operator==(const Set<T>::Iterator &other) const
        {
            return owner == other.owner && idx == other.idx;
        }

        bool operator!=(const Set<T>::Iterator &other) const
        {
            return owner != other.owner || idx != other.idx;
        }

        friend class Set<T>;
    };

    Set() : sizeIndex(0), nElements(0)
    {
        size = SetSizes[sizeIndex];
        AllocateMemory();
    }

    Set(const std::initializer_list<T> &l) : Set()
    {
        for (const auto &value : l)
            Insert(value);
    }

    Set(const Set<T> &other) : size(other.size), sizeIndex(other.sizeIndex), nElements(other.nElements)
    {
        AllocateMemory();
        for (size_t i = 0; i < other.size; i++)
        {
            if (!other.insertMarks[i] || other.deleteMarks[i])
                continue;
            Insert(other.data[i]);
        }
    }

    Set(Set<T> &&other)
        : size(other.size), sizeIndex(other.sizeIndex), nElements(other.nElements), data(other.data),
          insertMarks(other.insertMarks), deleteMarks(other.deleteMarks)
    {
        other.size = 0;
        other.sizeIndex = 0;
        other.nElements = 0;
        other.data = nullptr;
        other.insertMarks = nullptr;
        other.deleteMarks = nullptr;
    }

    Set<T> &operator=(const Set<T> &other)
    {
        ReleaseMemory();
        size = other.size;
        sizeIndex = other.sizeIndex;
        nElements = other.nElements;
        AllocateMemory();
        for (size_t i = 0; i < other.size; i++)
        {
            if (!other.insertMarks[i] || other.deleteMarks[i])
                continue;
            Insert(other.data[i]);
        }
        return *this;
    }

    Set<T> &operator=(const std::initializer_list<T> &l)
    {
        ReleaseMemory();
        *this = Set<T>(l);
        return *this;
    }

    Set<T> &operator=(Set<T> &&other) noexcept
    {
        size = other.size;
        sizeIndex = other.sizeIndex;
        nElements = other.nElements;
        data = other.data;
        insertMarks = other.insertMarks;
        deleteMarks = other.deleteMarks;

        other.size = 0;
        other.sizeIndex = 0;
        other.nElements = 0;
        other.data = nullptr;
        other.insertMarks = nullptr;
        other.deleteMarks = nullptr;
        return *this;
    }

    ~Set()
    {
        ReleaseMemory();
    }

    template <class ValueType> void Insert(ValueType &&value)
    {
        if (Contains(value))
            return;
        const auto idx = FindPosition(value, false);
        insertMarks[idx] = true;
        deleteMarks[idx] = false;
        data[idx] = Types::Forward<decltype(value)>(value);
        nElements++;
        DynamicallyResize();
    }

    void Remove(const T &value)
    {
        if (IsEmpty())
            return;
        const auto idx = FindPosition(value, true);
        if (!insertMarks[idx] || deleteMarks[idx] || data[idx] != value)
            return;
        deleteMarks[idx] = true;
        nElements--;
        DynamicallyResize();
    }

    void Clear()
    {
        *this = Set<T>();
    }

    bool Contains(const T &value) const
    {
        const auto idx = FindPosition(value, true);
        return !(!insertMarks[idx] || deleteMarks[idx] || data[idx] != value);
    }

    Set<T>::Iterator Find(const T &value) const
    {
        Set<T>::Iterator itr;
        itr.owner = (Set<T> *)this;
        const auto idx = FindPosition(value, true);
        if (insertMarks[idx] && !deleteMarks[idx] && data[idx] == value)
            itr.idx = idx;
        else
            return Last();
        return itr;
    }

    bool IsEmpty() const
    {
        return nElements == 0;
    }

    size_t Length() const
    {
        return nElements;
    }

    Set<T>::Iterator First() const
    {
        return Set<T>::Iterator((Set<T> *)this, false);
    }

    Set<T>::Iterator Last() const
    {
        return Set<T>::Iterator((Set<T> *)this, true);
    }

    T &operator[](const T &value)
    {
        Iterator itr = Find(value);
        if (itr == Last())
        {
            Insert(value);
            return *Find(value);
        }
        return *itr;
    }

    const T &operator[](const T &value) const
    {
        return *Find(value);
    }

    friend class Set<T>::Iterator;
};

template <class T> const size_t Set<T>::SetSizes[] = {13, 37, 79, 97, 199, 401, 857, 1699, 3307};

#endif // HASH_TABLE_HPP
