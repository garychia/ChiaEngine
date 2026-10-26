#ifndef WORLD_HPP
#define WORLD_HPP

#include "Data/DynamicArray.hpp"
#include "Data/Hash.hpp"
#include "Data/HashTable.hpp"
#include "ComponentPool.hpp"
#include "Entity.hpp"
#include "EntityRegistry.hpp"
#include "System/Module/TypeId.hpp"
#include "View.hpp"

// 世界的門面:Entity 生命週期 + 元件 CRUD + View 走訪。
// 元件池以 TypeId<T>() 為 key 註冊,與 EngineContext 共用型別 ID 計數器。
// World 是 Sim 層的核心資料結構 — 不碰 GPU、不讀牆鐘,完全可無頭測試。
class World
{
  private:
    EntityRegistry registry;
    DynamicArray<IComponentPool *> pools;
    HashTable<uint32_t, IComponentPool *> poolByType;

    template <class T> ComponentPool<T> *GetOrCreatePool()
    {
        ComponentPool<T> *pPool = GetPool<T>();
        if (pPool)
            return pPool;
        pPool = new ComponentPool<T>();
        pools.Append(pPool);
        poolByType.Insert(TypeId<T>(), pPool);
        return pPool;
    }

  public:
    World() : registry(), pools(), poolByType()
    {
    }

    ~World()
    {
        for (size_t i = 0; i < pools.GetNElements(); i++)
            delete pools[i];
    }

    World(const World &) = delete;
    World &operator=(const World &) = delete;

    // ---- Entity ----
    Entity CreateEntity()
    {
        return registry.Create();
    }

    void DestroyEntity(Entity entity)
    {
        if (!registry.Alive(entity))
            return;
        const uint32_t index = entity.GetIndex();
        for (size_t i = 0; i < pools.GetNElements(); i++)
            pools[i]->RemoveEntity(index);
        registry.Destroy(entity);
    }

    bool Alive(Entity entity) const
    {
        return registry.Alive(entity);
    }

    uint32_t GetEntityCount() const
    {
        return registry.GetNElements();
    }

    // entity index -> 完整 handle(View 用)
    Entity GetEntityByIndex(uint32_t entityIndex) const
    {
        return registry.GetEntityByIndex(entityIndex);
    }

    // ---- Component(public API 吃 Entity handle)----
    template <class T> T *AddComponent(Entity entity, const T &value)
    {
        return GetOrCreatePool<T>()->Add(entity.GetIndex(), value);
    }

    template <class T> T *GetComponent(Entity entity)
    {
        return GetComponentByIndex<T>(entity.GetIndex());
    }

    template <class T> bool HasComponent(Entity entity) const
    {
        return HasComponentByIndex<T>(entity.GetIndex());
    }

    template <class T> void RemoveComponent(Entity entity)
    {
        ComponentPool<T> *pPool = GetPool<T>();
        if (pPool)
            pPool->RemoveEntity(entity.GetIndex());
    }

    // ---- View ----
    template <class... Cs> ::View<Cs...> View()
    {
        return ::View<Cs...>(this);
    }

    // ---- 內部(View 用,吃 entity index)----
    template <class T> T *GetComponentByIndex(uint32_t entityIndex)
    {
        ComponentPool<T> *pPool = GetPool<T>();
        return pPool ? pPool->Get(entityIndex) : nullptr;
    }

    template <class T> bool HasComponentByIndex(uint32_t entityIndex) const
    {
        ComponentPool<T> *pPool = GetPool<T>();
        return pPool && pPool->Contains(entityIndex);
    }

    template <class T> ComponentPool<T> *GetPool() const
    {
        HashTable<uint32_t, IComponentPool *>::Iterator itr = poolByType.Find(TypeId<T>());
        if (itr == poolByType.Last())
            return nullptr;
        return static_cast<ComponentPool<T> *>(itr->Value());
    }

    // 整個世界狀態的確定性雜湊:entity 身分(存活集合 + 世代 + dense 走訪順序)+ 每個
    // 元件池(位置 + data + owners,依註冊順序)。兩個相同輸入的執行必須逐 bit 相同;
    // 任何狀態分歧都會反映在 hash 上。
    // 註:池註冊順序 = 首次 AddComponent<T> 的順序,對確定性 Sim 來說是確定的。
    //
    // #149:entity 側以前只摺入「數量」,所以「slot 從未回收」與「建了又刪再建」(世代 +1)
    // 同 hash —— 只差 handle 回收歷史的 desync 看不到。現在改摺 registry.Hash():
    // dense 走訪序 + 每筆完整 raw handle(世代 | index)。freeIndices 刻意不摺 ——
    // 它描述的是「未來的」配置順序,不是當前狀態。
    //
    // 摺入的是「池的位置」而不是 TypeId<T>():TypeId 是執行期第一次請求的計數器,
    // 會隨服務註冊順序變動(EngineContext 與 World 共用同一個計數器),餵進 hash 就等於
    // 讓 hash 依賴與世界狀態無關的執行順序 → 跨 build 比較失去意義(#141)。
    // 位置的來源是 Sim 程式碼本身(首次 AddComponent<T> 的順序),因此是確定性的。
    // 代價:型別身分不再進 hash(位置 + 位元組 + owners 就是完整輸入)。稽核「哪些型別」
    // 改由 hash 記錄承擔(World::WriteHashRecord(),#150),不靠 64-bit hash 反推。
    uint64_t Hash() const
    {
        uint64_t h = FNV1A64_OFFSET;
        h = FNV1A64(h, registry.Hash());
        for (size_t i = 0; i < pools.GetNElements(); i++)
        {
            h = FNV1A64(h, static_cast<uint32_t>(i)); // 池的位置:序列歧義不靠型別 id
            h = FNV1A64(h, pools[i]->Hash());
        }
        return h;
    }
};

// View 的方法需要 World 的完整定義,故在此(World 定義之後)引入實作。
#include "View.inl"

#endif // WORLD_HPP
