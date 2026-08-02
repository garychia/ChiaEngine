#ifndef COMPONENT_POOL_HPP
#define COMPONENT_POOL_HPP

#include "Data/DynamicArray.hpp"
#include "Data/Hash.hpp"
#include "Data/HashTable.hpp"
#include "System/Module/TypeId.hpp"

// 元件池的型別消除介面:World 用同一份陣列管理所有型別的池。
class IComponentPool
{
  public:
    virtual ~IComponentPool() = default;

    virtual uint32_t GetTypeId() const = 0;

    // 池內容的確定性雜湊(FNV-1a 64):replay 驗證用。
    // 前提:元件是 trivially copyable,且儲存時 padding 已歸零(見 Add)。
    virtual uint64_t Hash() const = 0;

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
        // 先放零初始化的 slot,再成員指派 — padding 位元組保證為零,
        // 讓 Hash() 對原始位元組雜湊時不會吃到未定義的記憶體。
        T slot{};
        slot = value;
        data.Append(slot);
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

    uint64_t Hash() const override
    {
        uint64_t h = FNV1A64_OFFSET;
        h = FNV1A64(h, GetTypeId());
        for (size_t i = 0; i < data.GetNElements(); i++)
            h = FNV1A64(h, &data[i], sizeof(T));
        for (size_t i = 0; i < owners.GetNElements(); i++)
            h = FNV1A64(h, owners[i]);
        return h;
    }
};

#endif // COMPONENT_POOL_HPP
