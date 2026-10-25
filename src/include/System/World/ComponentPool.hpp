#ifndef COMPONENT_POOL_HPP
#define COMPONENT_POOL_HPP

#include "Data/DynamicArray.hpp"
#include "Data/Hash.hpp"
#include "Data/HashTable.hpp"
#include "System/Module/TypeId.hpp"

#include <cstring> // std::memset

// 元件池的型別消除介面:World 用同一份陣列管理所有型別的池。
class IComponentPool
{
  public:
    virtual ~IComponentPool() = default;

    virtual uint32_t GetTypeId() const = 0;

    // 池內容的確定性雜湊(FNV-1a 64):replay 驗證用。
    // 前提:元件是 trivially copyable,且型別沒有隱含 padding(見 Add 與 #129)。
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
        // Hash() 逐 bit 讀 sizeof(T),所以這裡要讓「每個成員」都被寫成確定值:
        // 先 Append 佔位,再 memset 整塊儲存,最後指派 —— 成員一定被寫過,陣列
        // 容量的未初始化記憶體不會漏進成員位元組(空 struct 實測會留垃圾),
        // 「同值 → 同 hash」因此成立。
        // #129:但這「不能」保證隱含 padding 為零。trivially copyable 的指派允許
        // 被實作成整塊複製(padding 的值是 unspecified、對 conforming program
        // 無從觀察,as-if 規則放行),來源暫時物件的 padding 會蓋掉剛清零的位元組
        // —— gcc 與 clang 就此不同,甚至同一份 build 的兩次執行都可能不同。
        // 契約:元件型別請保持無隱含 padding(需要填充就宣告明確的成員,沒被指名
        // 的陣列成員由聚合初始化值初始化為 0,是標準保證的),否則改用 per-type
        // hasher。詳見 #129。
        data.Append(value);
        std::memset(&data.GetLast(), 0, sizeof(T));
        data.GetLast() = value;
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
