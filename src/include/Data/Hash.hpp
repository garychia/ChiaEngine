#ifndef HASH_HPP
#define HASH_HPP

#include <cstddef>
#include <cstdint>

// FNV-1a 64-bit:輕量、確定性的位元組流雜湊。
// 用於 Sim 狀態快照(World::Hash)與未來檔案格式 — 不是加密用途。
// 保證:相同位元組流 → 相同 hash(同一 binary 內)。
inline constexpr uint64_t FNV1A64_OFFSET = 14695981039346656037ull;
inline constexpr uint64_t FNV1A64_PRIME = 1099511628211ull;

inline uint64_t FNV1A64(uint64_t hash, const void *pData, size_t nBytes)
{
    const uint8_t *pBytes = static_cast<const uint8_t *>(pData);
    for (size_t i = 0; i < nBytes; i++)
    {
        hash ^= pBytes[i];
        hash *= FNV1A64_PRIME;
    }
    return hash;
}

template <class T> uint64_t FNV1A64(uint64_t hash, const T &value)
{
    return FNV1A64(hash, &value, sizeof(T));
}

#endif // HASH_HPP
