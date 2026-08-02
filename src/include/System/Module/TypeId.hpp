#ifndef TYPE_ID_HPP
#define TYPE_ID_HPP

#include <cstdint>

// 編譯期型別 ID:每個 T 拿到一個唯一 u32,無 RTTI。
// EngineContext 的服務表與 World 的元件池共用同一組計數器,
// 保證整個引擎內型別 ID 不衝突。
inline uint32_t NextTypeId()
{
    static uint32_t next = 1;
    return next++;
}

template <class T> uint32_t TypeId()
{
    static const uint32_t id = NextTypeId();
    return id;
}

#endif // TYPE_ID_HPP
