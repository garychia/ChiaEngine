#ifndef ENTITY_REGISTRY_HPP
#define ENTITY_REGISTRY_HPP

#include "Data/DynamicArray.hpp"
#include "Data/Hash.hpp" // FNV1A64:狀態雜湊(#149)
#include "Data/HashTable.hpp"
#include "Entity.hpp"

// Sparse set 實作,全部建在自己的容器上:
// - dense   : 存活的 entity 完整 handle(raw)平鋪(DynamicArray),走訪用
// - sparse  : entity index -> dense 位置(HashTable)
// - alive   : slot 是否被佔用(DynamicArray<bool>)
// - generations : slot 目前的世代(DynamicArray<uint32_t>)
// - freeIndices : 可重用的 slot(DynamicArray)
// Create / Destroy 都是 O(1) amortized;Iterate 是連續記憶體,快取友善。
class EntityRegistry
{
  private:
    DynamicArray<uint32_t> generations;
    DynamicArray<bool> alive;
    DynamicArray<uint32_t> dense; // 完整 raw handle(generation | index)
    HashTable<uint32_t, uint32_t> sparse;
    DynamicArray<uint32_t> freeIndices;

  public:
    EntityRegistry()
        : generations(), alive(), dense(), sparse(), freeIndices()
    {
    }

    Entity Create()
    {
        uint32_t index;
        uint32_t generation;
        if (!freeIndices.IsEmpty())
        {
            index = freeIndices.GetLast();
            freeIndices.RemoveLast();
            generation = generations[index] + 1; // 重用 slot,世代 +1 → 舊 handle 失效
        }
        else
        {
            index = static_cast<uint32_t>(generations.GetNElements());
            generations.Append(1);
            alive.Append(true);
            generation = 1;
        }
        generations[index] = generation;
        alive[index] = true;
        const uint32_t raw = (generation << Entity::INDEX_BITS) | index;
        sparse.Insert(index, static_cast<uint32_t>(dense.GetNElements()));
        dense.Append(raw);
        return Entity::FromRaw(raw);
    }

    void Destroy(Entity entity)
    {
        if (!Alive(entity))
            return;
        const uint32_t index = entity.GetIndex();
        alive[index] = false;
        freeIndices.Append(index);

        // dense swap-remove:與最後一個交換,再更新被搬動者的 sparse 位置
        HashTable<uint32_t, uint32_t>::Iterator itr = sparse.Find(index);
        const uint32_t pos = itr->Value();
        const uint32_t lastRaw = dense.GetLast();
        dense[pos] = lastRaw;
        sparse.Insert(lastRaw & Entity::INDEX_MASK, pos);
        dense.RemoveLast();
        sparse.Remove(index);
    }

    bool Alive(Entity entity) const
    {
        const uint32_t index = entity.GetIndex();
        if (index >= alive.GetNElements())
            return false;
        return alive[index] && generations[index] == entity.GetGeneration();
    }

    // 存活數量
    uint32_t GetNElements() const
    {
        return static_cast<uint32_t>(dense.GetNElements());
    }

    // 狀態雜湊:每個存活 entity 的完整 raw handle(generation | index),照 dense 的順序。
    //
    // 為什麼需要(#149):World::Hash() 以前只摺入 entity「數量」,所以
    // 「slot 0 從未被回收」與「slot 0 建了又刪再建(世代 1 → 2)」—— 元件位元組完全相同
    // —— 會產生同一個 hash。只差 handle 回收歷史的 desync 因此看不到,而 stale handle
    // 的 Alive() 結果在兩個 state 之間是不一樣的。
    // dense 的順序同時是 View 走訪順序,所以順序本身也進 hash(它是可觀察的)。
    // freeIndices 不摺入:它只影響「下一個 entity 拿到哪個 slot」,那個結果會由之後的
    // Create 反映在 dense/generations 上;若哪天 sim 直接觀察 free-list 順序,再補。
    uint64_t Hash() const
    {
        uint64_t h = FNV1A64_OFFSET;
        for (size_t i = 0; i < dense.GetNElements(); i++)
            h = FNV1A64(h, dense[i]);
        return h;
    }

    // dense 位置 -> 完整 handle(供走訪)
    Entity At(uint32_t pos) const
    {
        return Entity::FromRaw(dense[pos]);
    }

    // entity index -> 完整 handle(含世代);不存在回傳無效 handle
    Entity GetEntityByIndex(uint32_t index) const
    {
        HashTable<uint32_t, uint32_t>::Iterator itr = sparse.Find(index);
        if (itr == sparse.Last())
            return Entity::FromRaw(0);
        return Entity::FromRaw(dense[itr->Value()]);
    }
};

#endif // ENTITY_REGISTRY_HPP
