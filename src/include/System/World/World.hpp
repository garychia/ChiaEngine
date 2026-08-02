#ifndef WORLD_HPP
#define WORLD_HPP

#include "Data/DynamicArray.hpp"
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
};

// View 的方法需要 World 的完整定義,故在此(World 定義之後)引入實作。
#include "View.inl"

#endif // WORLD_HPP
