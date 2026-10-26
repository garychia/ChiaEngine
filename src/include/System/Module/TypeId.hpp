#ifndef TYPE_ID_HPP
#define TYPE_ID_HPP

#include <atomic>
#include <cstdint>

// 編譯期型別 ID:每個 T 拿到一個唯一 u32,無 RTTI。
// EngineContext 的服務表與 World 的元件池共用同一組計數器,
// 保證整個引擎內型別 ID 不衝突。
//
// ⚠ 這個數字是「執行期第一次請求的順序」,不是型別的穩定身分:它不是編譯期常數,
// 會隨 module attach 順序 / 服務註冊數量 / 哪條程式路徑先跑到而變動,而且每份 binary
// 各有一組。所以:
//   - 可以當 World / EngineContext 的查表 key(functional identity);
//   - 不可以餵進 World::Hash() 或任何要跨 build 比較的狀態雜湊(見 #141)。
//     ComponentPool::Hash() 已不再雜湊這個 id,World::Hash() 改摺入池的註冊序號。
inline uint32_t NextTypeId()
{
    // atomic:JobSystem 的執行緒可能同時第一次請求各自的 TypeId(#141)。
    static std::atomic<uint32_t> next{1};
    return next.fetch_add(1, std::memory_order_relaxed);
}

template <class T> uint32_t TypeId()
{
    static const uint32_t id = NextTypeId();
    return id;
}

#endif // TYPE_ID_HPP
