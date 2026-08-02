#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <cstdint>

// Entity 是 u32 handle:低 24 bit 索引 + 高 8 bit 世代。
// 世代讓「已銷毀的 handle」立刻失效(Alive 檢查),防止懸空引用。
// 注意:同一 slot 重用超過 256 次後世代會繞回 — 引擎規模下可忽略。
struct Entity
{
  public:
    static constexpr uint32_t INDEX_BITS = 24;
    static constexpr uint32_t INDEX_MASK = 0x00FFFFFFu;

    static Entity FromRaw(uint32_t raw)
    {
        Entity entity;
        entity.raw = raw;
        return entity;
    }

    uint32_t GetIndex() const
    {
        return raw & INDEX_MASK;
    }

    uint32_t GetGeneration() const
    {
        return raw >> INDEX_BITS;
    }

    uint32_t GetRaw() const
    {
        return raw;
    }

    bool operator==(const Entity &other) const
    {
        return raw == other.raw;
    }

    bool operator!=(const Entity &other) const
    {
        return raw != other.raw;
    }

  private:
    uint32_t raw = 0;
};

#endif // ENTITY_HPP
