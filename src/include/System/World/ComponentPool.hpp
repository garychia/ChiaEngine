#ifndef COMPONENT_POOL_HPP
#define COMPONENT_POOL_HPP

#include "Data/DynamicArray.hpp"
#include "Data/Hash.hpp"
#include "Data/HashTable.hpp"
#include "System/Module/TypeId.hpp"

#include <type_traits>

// 元件池的型別消除介面:World 用同一份陣列管理所有型別的池。
class IComponentPool
{
  public:
    virtual ~IComponentPool() = default;

    virtual uint32_t GetTypeId() const = 0;

    // 池內容的確定性雜湊(FNV-1a 64):replay 驗證用。
    // 前提:元件型別沒有隱含 padding(用 CHIA_PADDING_FREE 在型別定義處把關,
    // 見 Data/Padding.hpp 與 #129 / #142)。有 padding 的型別請改用明確的填充成員,
    // 或提供 HashComponent<T> 專案化。
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
        // Hash() 逐位元組讀 sizeof(T) → 池中每個元素的所有 byte 都必須有確定的值。
        // 型別沒有隱含 padding(CHIA_PADDING_FREE 保證)時,複製建構/指派會寫到
        // 每一個 byte,所以直接指派就够了。
        //
        // 舊版寫法 `data.Append(value); memset(&GetLast(), 0, sizeof(T)); GetLast() = value;`
        // 是無效的:memset 緊接著被整塊指派覆蓋掉(死碼),padding 的值其實來自來源
        // 物件的 padding。clang -O2 保留那些垃圾 byte、gcc -O2 恰好清零 —— 「靠 memset
        // 保證確定性」會讓 hash 依編譯器而異,這正是 #129 / #142 要消滅的東西。
        data.Append(value);
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
        // 刻意不雜湊 GetTypeId():它是執行期「第一次請求」的計數器,會隨服務註冊順序
        // 變動、每份 binary 各一組 → 餵進 hash 會讓跨 build 的比較失去意義(#141)。
        // 型別之間的區分由 World::Hash() 摺入的池位置承擔;型別清單另存於 hash 記錄
        // (World::WriteHashRecord(),#150)。
        //
        // 空型別(tag 元件)沒有成員:那 1 byte 的儲存值 unspecified,讀它會讓「同值」
        // 的兩個 world 產生不同 hash(建構路徑不同 → 殘值不同)。所以刻意不讀它 ——
        // 空型別池的內容完全由 owners 與池位置決定。
        if constexpr (!std::is_empty_v<T>)
        {
            for (size_t i = 0; i < data.GetNElements(); i++)
                h = FNV1A64(h, &data[i], sizeof(T));
        }
        for (size_t i = 0; i < owners.GetNElements(); i++)
            h = FNV1A64(h, owners[i]);
        return h;
    }
};

#endif // COMPONENT_POOL_HPP
