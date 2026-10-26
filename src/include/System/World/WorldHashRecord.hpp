#ifndef WORLD_HASH_RECORD_HPP
#define WORLD_HASH_RECORD_HPP

#include "Data/DynamicArray.hpp"

#include <cstdint>
#include <ostream>
#include <string_view>

// World::Hash() 的「配方」版本。任何改變 hash 輸入的改動都要升這個號 ——
// 否則舊的 replay 記錄會靜默失去意義:同一個 64-bit 值不再代表同一件事。
// 「hash 是 source of truth」的前提,是記錄裡寫清楚它是用哪一版算出來的(#150)。
//
// v1 (main, 2026-10):entity 數 + 每個元件池的 (TypeId<T>() + data bytes + owners),
//                     池依註冊順序。
// v2 (隨 #147 / #149 落地):registry 身分(dense raw handle:世代 + index)+ 每個池的
//                     (data + owners) + 池的註冊序號;不再含 TypeId。
//                     ⚠ 這兩支 PR 本人要負責把這個常數改成 2 並更新本註解。
inline constexpr uint32_t CHIA_WORLD_HASH_RECIPE_VERSION = 1;

// 記錄裡的一個元件池;註冊順序由它在陣列中的位置表示。
struct HashRecordPool
{
    uint32_t typeId = 0;       // 執行期 key:只在同一個 process 內有意義
    std::string_view typeName; // 稽核用;不進 hash(見 Data/TypeName.hpp)
    uint32_t elementCount = 0;
};

// 自我描述的指紋:hash 值 + 產生它的配方 + 註冊型別清單(依註冊順序)。
//
// 為什麼需要:#147 之後型別身分不在 digest 裡,#149 之後 registry 身分才在 ——
// 所以「兩個 hash 相同」回答不了「是不是同一個配方、同一組型別」。這份記錄就是那個答案,
// 而且是給人與 CI 讀的稽核資料,不是 hash 的輸入。
struct WorldHashRecord
{
    uint32_t recipeVersion = 0;
    uint64_t hash = 0;
    uint32_t entityCount = 0;
    uint32_t poolCount = 0;
    DynamicArray<HashRecordPool> pools; // 註冊順序
};

// 文字形式(ASCII、一行一筆):放進 replay 檔或 CI artifact 後可以直接 diff、肉眼讀。
inline void WriteWorldHashRecord(std::ostream &out, const WorldHashRecord &record)
{
    out << "chia-world-hash v" << record.recipeVersion << "\n";
    out << "hash " << std::hex << record.hash << std::dec << "\n";
    out << "entities " << record.entityCount << "\n";
    out << "pools " << record.poolCount << "\n";
    for (size_t i = 0; i < record.pools.GetNElements(); i++)
    {
        const HashRecordPool &entry = record.pools[i];
        out << "pool " << i << " typeid " << entry.typeId << " elements " << entry.elementCount << " name "
            << (entry.typeName.empty() ? std::string_view("<unknown>") : entry.typeName) << "\n";
    }
}

#endif // WORLD_HASH_RECORD_HPP
