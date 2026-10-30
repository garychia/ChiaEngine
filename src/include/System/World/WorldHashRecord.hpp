#ifndef WORLD_HASH_RECORD_HPP
#define WORLD_HASH_RECORD_HPP

#include "Data/DynamicArray.hpp"

#include <charconv>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

// World::Hash() 的「配方」版本。任何改變 hash 輸入的改動都要升這個號 ——
// 否則舊的 replay 記錄會靜默失去意義:同一個 64-bit 值不再代表同一件事。
// 「hash 是 source of truth」的前提,是記錄裡寫清楚它是用哪一版算出來的(#150)。
//
// v1 (已淘汰):entity 數 + 每個元件池的 (TypeId<T>() + data bytes + owners),池依註冊順序。
//              這個配方不可靠:TypeId 是執行期「第一次請求」的計數器,值隨服務註冊順序
//              變動、每份 binary 各一組,於是同一份邏輯狀態在不同 build 會得到不同 hash
//              (#141)。指紋編號從 6eb9a3a05706dd54 變成 115770ae32637597 的那次就是它。
// v2 (現行 main,#147 + #149):registry 身分(dense 走訪序 + 每筆完整 raw handle:
//              世代 | index)+ 每個元件池的 (位置 + data bytes + owners);不含 TypeId。
inline constexpr uint32_t CHIA_WORLD_HASH_RECIPE_VERSION = 2;

// 記錄裡的一個元件池;註冊順序由它在陣列中的位置表示。
struct HashRecordPool
{
    uint32_t typeId = 0;       // 執行期 key:只在同一個 process 內有意義
    std::string typeName;      // 稽核用;不進 hash(見 Data/TypeName.hpp)
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
            << (entry.typeName.empty() ? std::string("<unknown>") : entry.typeName) << "\n";
    }
}

// 讀回 WriteWorldHashRecord 寫出的區塊(逐行、嚴格)。不驗證 recipe version ——
// 那是 ReadReplayFile 的責任,這裡只回報解析到的值。line 為 1-based:進入時等於
// chia-world-hash 那一行,每成功消耗一行就 +1,失敗時停在出錯的那一行。
enum class HashRecordReadResult : uint8_t
{
    Ok,
    Malformed,
    Truncated
};

namespace worldhashrecord_detail
{
inline bool ReadLine(std::istream &in, std::string &line) noexcept
{
    if (!std::getline(in, line))
        return false;
    if (!line.empty() && line.back() == '\r')
        line.pop_back();
    return true;
}

// 取出下一個以單一 ASCII 空格分隔的 token;開頭/結尾/連續空格與 tab 都算錯。
inline bool NextToken(std::string_view line, size_t &pos, std::string_view &token) noexcept
{
    if (pos >= line.size() || line[pos] == ' ')
        return false;
    const size_t separator = line.find(' ', pos);
    const size_t end = (separator == std::string_view::npos) ? line.size() : separator;
    token = line.substr(pos, end - pos);
    for (char c : token)
    {
        if (c == '\t' || c == '\r')
            return false;
    }
    if (end == line.size())
    {
        pos = line.size();
        return true;
    }
    if (end + 1 >= line.size() || line[end + 1] == ' ')
        return false;
    pos = end + 1;
    return true;
}

inline bool SplitLine(std::string_view line, std::string_view *tokens, size_t capacity, size_t &count) noexcept
{
    count = 0;
    size_t pos = 0;
    while (pos < line.size())
    {
        if (count >= capacity)
            return false;
        if (!NextToken(line, pos, tokens[count]))
            return false;
        ++count;
    }
    return true;
}

inline bool ParseUInt32(std::string_view token, uint32_t &out) noexcept
{
    if (token.empty())
        return false;
    uint32_t value = 0;
    const std::from_chars_result result = std::from_chars(token.data(), token.data() + token.size(), value);
    if (result.ec != std::errc() || result.ptr != token.data() + token.size())
        return false;
    out = value;
    return true;
}

inline bool IsLowerHex(char c) noexcept
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

inline bool ParseHexUInt64(std::string_view token, uint64_t &out) noexcept
{
    if (token.empty() || token.size() > 16)
        return false;
    for (char c : token)
    {
        if (!IsLowerHex(c))
            return false;
    }
    uint64_t value = 0;
    const std::from_chars_result result = std::from_chars(token.data(), token.data() + token.size(), value, 16);
    if (result.ec != std::errc() || result.ptr != token.data() + token.size())
        return false;
    out = value;
    return true;
}
} // namespace worldhashrecord_detail

inline HashRecordReadResult ReadWorldHashRecord(std::istream &in, WorldHashRecord &outRecord,
                                                uint32_t &inOutLineNumber)
{
    using namespace worldhashrecord_detail;

    std::string line;
    std::string_view tokens[4];
    size_t count = 0;
    WorldHashRecord parsed;

    if (!ReadLine(in, line))
        return HashRecordReadResult::Truncated;
    if (!SplitLine(line, tokens, 4, count) || count != 2 || tokens[0] != "chia-world-hash")
        return HashRecordReadResult::Malformed;
    if (tokens[1].size() < 2 || tokens[1][0] != 'v' || !ParseUInt32(tokens[1].substr(1), parsed.recipeVersion))
        return HashRecordReadResult::Malformed;
    ++inOutLineNumber;

    if (!ReadLine(in, line))
        return HashRecordReadResult::Truncated;
    if (!SplitLine(line, tokens, 3, count) || count != 2 || tokens[0] != "hash" || !ParseHexUInt64(tokens[1], parsed.hash))
        return HashRecordReadResult::Malformed;
    ++inOutLineNumber;

    if (!ReadLine(in, line))
        return HashRecordReadResult::Truncated;
    if (!SplitLine(line, tokens, 3, count) || count != 2 || tokens[0] != "entities" ||
        !ParseUInt32(tokens[1], parsed.entityCount))
        return HashRecordReadResult::Malformed;
    ++inOutLineNumber;

    if (!ReadLine(in, line))
        return HashRecordReadResult::Truncated;
    if (!SplitLine(line, tokens, 3, count) || count != 2 || tokens[0] != "pools" ||
        !ParseUInt32(tokens[1], parsed.poolCount))
        return HashRecordReadResult::Malformed;
    ++inOutLineNumber;

    for (uint32_t i = 0; i < parsed.poolCount; ++i)
    {
        if (!ReadLine(in, line))
            return HashRecordReadResult::Truncated;

        size_t pos = 0;
        std::string_view token;
        uint32_t ordinal = 0;
        HashRecordPool entry;
        if (!NextToken(line, pos, token) || token != "pool")
            return HashRecordReadResult::Malformed;
        if (!NextToken(line, pos, token) || !ParseUInt32(token, ordinal) || ordinal != i)
            return HashRecordReadResult::Malformed;
        if (!NextToken(line, pos, token) || token != "typeid")
            return HashRecordReadResult::Malformed;
        if (!NextToken(line, pos, token) || !ParseUInt32(token, entry.typeId))
            return HashRecordReadResult::Malformed;
        if (!NextToken(line, pos, token) || token != "elements")
            return HashRecordReadResult::Malformed;
        if (!NextToken(line, pos, token) || !ParseUInt32(token, entry.elementCount))
            return HashRecordReadResult::Malformed;
        if (!NextToken(line, pos, token) || token != "name" || pos >= line.size())
            return HashRecordReadResult::Malformed;

        const std::string_view name = std::string_view(line).substr(pos);
        if (name != "<unknown>")
            entry.typeName.assign(name.data(), name.size());
        parsed.pools.Append(entry);
        ++inOutLineNumber;
    }

    outRecord = parsed;
    return HashRecordReadResult::Ok;
}

#endif // WORLD_HASH_RECORD_HPP
