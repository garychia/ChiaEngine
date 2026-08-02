#ifndef VIEW_HPP
#define VIEW_HPP

#include "ComponentPool.hpp"

#include <cstddef>
#include <cstdint>
#include <tuple>

class World;

// 走訪同時擁有 Components... 全部元件的 entity。
// Entry 是 std::tuple<Entity, T*...>,配合 structured binding:
//
//     for (auto [entity, pos, vel] : world.View<Position, Velocity>())
//     {
//         pos->x += vel->x * dt;
//     }
//
// 方法實作在 View.inl(需要 World 完整定義),由 World.hpp 在 class 之後引入。
template <class... Components> class View
{
  public:
    using Entry = std::tuple<Entity, Components *...>;

  private:
    template <class First, class... Rest> struct FirstOf
    {
        using Type = First;
    };

    using DriverType = typename FirstOf<Components...>::Type;

    World *pWorld;
    ComponentPool<DriverType> *pDriver; // 驅動 pool:走訪它的連續記憶體

    template <class C> bool HasAll(uint32_t entityIndex) const;

    bool AllComponentsPresent(uint32_t entityIndex) const;

    template <class C> C *GetComponentPtr(uint32_t entityIndex) const;

    Entry MakeEntry(uint32_t entityIndex) const;

  public:
    explicit View(World *pWorld);

    class Iterator
    {
      private:
        View *pView;
        size_t position;

        bool Valid() const;

        void SkipInvalid();

      public:
        Iterator(View *pView, size_t position);

        Entry operator*() const;

        Iterator &operator++();

        bool operator==(const Iterator &other) const;

        bool operator!=(const Iterator &other) const;
    };

    Iterator begin();

    Iterator end();
};

#endif // VIEW_HPP
