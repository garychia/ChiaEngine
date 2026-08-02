#ifndef COMPONENT_POOL_HPP
#define COMPONENT_POOL_HPP

#include "Data/DynamicArray.hpp"
#include "Data/HashTable.hpp"
#include "System/Module/TypeId.hpp"

// 元件池的型別消除介面:World 用同一份陣列管理所有型別的池。
class IComponentPool
{
  public:
    virtual ~IComponentPool() = default;

    virtual uint32_t GetTypeId() const = 0;

    // Entity 銷毀時清掉它的元件
    virtual void RemoveEntity(uint32_t entityIndex) = 0;
};

// SoA 元件池:資料連續平鋪(DynamicArray<T>),配合 owner 對應回 entity。
// lookup: entity index -> data 位置,Add/Get/Remove 都是 O(1) amortized。
// Remove 用 swap-remove(與最後一個交換),並更新被搬動者的 lookup。
template <class T> class ComponentPool : public IComponentPool
{
  private:
    DynamicArray<T> data;
    DynamicArray<uint32_t> owners;
    HashTable<uint32_t, uint32_t> lookup;

  public:
    ComponentPool() : data(), owners(), lookup()
    {
    }

    uint32_t GetTypeId() const override
    {
        return TypeId<T>();
    }

    void RemoveEntity(uint32_t entityIndex) override
    {
        HashTable<uint32_t, uint32_t>::Iterator itr = lookup.Find(entityIndex);
        if (itr == lookup.Last())
            return;
        const uint32_t pos = itr->Value();
        const uint32_t lastOwner = owners.GetLast();
        data[pos] = data.GetLast();
        owners[pos] = lastOwner;
        lookup.Insert(lastOwner, pos);
        data.RemoveLast();
        owners.RemoveLast();
        lookup.Remove(entityIndex);
    }

    T *Add(uint32_t entityIndex, const T &value)
    {
        if (Contains(entityIndex))
            return Get(entityIndex); // 已存在,回傳現有(冪等)
        lookup.Insert(entityIndex, static_cast<uint32_t>(data.GetNElements()));
        data.Append(value);
        owners.Append(entityIndex);
        return &data.GetLast();
    }

    T *Get(uint32_t entityIndex)
    {
        HashTable<uint32_t, uint32_t>::Iterator itr = lookup.Find(entityIndex);
        if (itr == lookup.Last())
            return nullptr;
        return &data[itr->Value()];
    }

    bool Contains(uint32_t entityIndex) const
    {
        return lookup.Contains(entityIndex);
    }

    uint32_t GetNElements() const
    {
        return static_cast<uint32_t>(data.GetNElements());
    }

    uint32_t OwnerAt(uint32_t pos) const
    {
        return owners[pos];
    }
};

#endif // COMPONENT_POOL_HPP
