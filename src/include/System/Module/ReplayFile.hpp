#ifndef REPLAY_FILE_HPP
#define REPLAY_FILE_HPP

#include "Data/DynamicArray.hpp"
#include "System/Module/SimInput.hpp"
#include "System/World/WorldHashRecord.hpp"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

// Replay File Format v1:容器行、hash record 區塊、build identity、逐 tick 輸入、結束行。
// 純文字、一行一筆、只用 C++17 標準程式庫;解析完全不看 locale(整數走 from_chars)。

namespace replayfile_detail
{
constexpr std::string_view VersionBeforeSpace(std::string_view text) noexcept
{
    const size_t space = text.find(' ');
    return space == std::string_view::npos ? text : text.substr(0, space);
}
} // namespace replayfile_detail

// Build identity 只做稽核:寫出、解析、回報給呼叫端,絕不用來讓載入失敗。
#define CHIA_REPLAY_STRINGIFY_IMPL(value) #value
#define CHIA_REPLAY_STRINGIFY(value) CHIA_REPLAY_STRINGIFY_IMPL(value)
#define CHIA_REPLAY_GCC_VERSION                                                                                        \
    CHIA_REPLAY_STRINGIFY(__GNUC__) "." CHIA_REPLAY_STRINGIFY(__GNUC_MINOR__) "." CHIA_REPLAY_STRINGIFY(__GNUC_PATCHLEVEL__)
#define CHIA_REPLAY_MSVC_VERSION CHIA_REPLAY_STRINGIFY(_MSC_VER)

#if defined(__clang__)
inline constexpr std::string_view CHIA_REPLAY_BUILD_COMPILER = "clang";
inline constexpr std::string_view CHIA_REPLAY_BUILD_VERSION = replayfile_detail::VersionBeforeSpace(__clang_version__);
#elif defined(__GNUC__)
inline constexpr std::string_view CHIA_REPLAY_BUILD_COMPILER = "gcc";
inline constexpr std::string_view CHIA_REPLAY_BUILD_VERSION = CHIA_REPLAY_GCC_VERSION;
#elif defined(_MSC_VER)
inline constexpr std::string_view CHIA_REPLAY_BUILD_COMPILER = "msvc";
inline constexpr std::string_view CHIA_REPLAY_BUILD_VERSION = CHIA_REPLAY_MSVC_VERSION;
#else
inline constexpr std::string_view CHIA_REPLAY_BUILD_COMPILER = "unknown";
inline constexpr std::string_view CHIA_REPLAY_BUILD_VERSION = "unknown";
#endif
inline constexpr std::string_view CHIA_REPLAY_BUILD_FP_POLICY = "ffp-contract=off";

#undef CHIA_REPLAY_MSVC_VERSION
#undef CHIA_REPLAY_GCC_VERSION
#undef CHIA_REPLAY_STRINGIFY
#undef CHIA_REPLAY_STRINGIFY_IMPL

enum class ReplayFileResult : uint8_t
{
    Ok,
    BadContainer,
    RecipeMismatch,
    Malformed,
    Truncated
};

// build 行的三個 token:讓呼叫端能看出「這個檔來自別的 build」,格式本身不判定是不是錯。
struct ReplayBuildIdentity
{
    std::string compiler;
    std::string version;
    std::string fpPolicy;
};

struct ReplayFileError
{
    ReplayFileResult result = ReplayFileResult::Ok;
    uint32_t containerVersion = 0; // parsed from line 1; 0 if line 1 was not a container line
    uint32_t fileRecipe = 0;       // RecipeMismatch only
    uint32_t binaryRecipe = 0;     // RecipeMismatch only; CHIA_WORLD_HASH_RECIPE_VERSION
    uint32_t line = 1;             // 1-based line the load failed on
};

namespace replayfile_detail
{
inline void WriteDecimal(std::ostream &out, uint32_t value)
{
    char buffer[10];
    const std::to_chars_result result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    out.write(buffer, static_cast<std::streamsize>(result.ptr - buffer));
}

inline void WriteHexWord(std::ostream &out, uint32_t value)
{
    static constexpr char digits[] = "0123456789abcdef";
    char buffer[8];
    for (int i = 7; i >= 0; --i)
    {
        buffer[i] = digits[value & 0xFu];
        value >>= 4;
    }
    out.write(buffer, sizeof(buffer));
}

inline bool ParseHexWord(std::string_view token, uint32_t &out) noexcept
{
    if (token.size() != 8)
        return false;
    for (char c : token)
    {
        if (!worldhashrecord_detail::IsLowerHex(c))
            return false;
    }
    uint32_t value = 0;
    const std::from_chars_result result = std::from_chars(token.data(), token.data() + token.size(), value, 16);
    if (result.ec != std::errc() || result.ptr != token.data() + token.size())
        return false;
    out = value;
    return true;
}
} // namespace replayfile_detail

inline bool WriteReplayFile(std::ostream &out, const WorldHashRecord &record, const DynamicArray<SimInput> &log)
{
    out << "chia-replay v1\n";
    WriteWorldHashRecord(out, record);
    out << "build " << CHIA_REPLAY_BUILD_COMPILER << " " << CHIA_REPLAY_BUILD_VERSION << " " << CHIA_REPLAY_BUILD_FP_POLICY
        << "\n";
    out << "ticks ";
    replayfile_detail::WriteDecimal(out, static_cast<uint32_t>(log.GetNElements()));
    out << "\n";

    for (size_t i = 0; i < log.GetNElements(); ++i)
    {
        const SimInput &input = log[i];
        uint32_t axisX = 0;
        uint32_t axisY = 0;
        std::memcpy(&axisX, &input.axisX, sizeof(axisX));
        std::memcpy(&axisY, &input.axisY, sizeof(axisY));

        out << "tick ";
        replayfile_detail::WriteHexWord(out, input.actionBits);
        out << " ";
        replayfile_detail::WriteHexWord(out, axisX);
        out << " ";
        replayfile_detail::WriteHexWord(out, axisY);
        out << "\n";
    }

    out << "end\n";
    return out.good();
}

inline ReplayFileError ReadReplayFile(std::istream &in, WorldHashRecord &outRecord, DynamicArray<SimInput> &outLog,
                                     ReplayBuildIdentity &outBuild)
{
    using namespace worldhashrecord_detail;

    ReplayFileError error;
    uint32_t line = 1;
    std::string text;
    std::string_view tokens[4];
    size_t count = 0;

    if (!ReadLine(in, text))
    {
        error.result = ReplayFileResult::Truncated;
        error.line = line;
        return error;
    }

    const bool splitOk = SplitLine(text, tokens, 4, count);
    if (splitOk && count >= 2 && tokens[0] == "chia-replay" && tokens[1].size() >= 2 && tokens[1][0] == 'v')
    {
        uint32_t containerVersion = 0;
        if (ParseUInt32(tokens[1].substr(1), containerVersion))
            error.containerVersion = containerVersion;
    }
    if (!splitOk || count != 2 || tokens[0] != "chia-replay" || tokens[1] != "v1")
    {
        error.result = ReplayFileResult::BadContainer;
        error.line = line;
        return error;
    }
    ++line;

    const uint32_t recipeLine = line;
    uint32_t recordLine = line;
    WorldHashRecord record;
    const HashRecordReadResult recordResult = ReadWorldHashRecord(in, record, recordLine);
    if (recordResult == HashRecordReadResult::Truncated)
    {
        error.result = ReplayFileResult::Truncated;
        error.line = recordLine;
        return error;
    }
    if (recordResult == HashRecordReadResult::Malformed)
    {
        error.result = ReplayFileResult::Malformed;
        error.line = recordLine;
        return error;
    }
    line = recordLine;

    if (!ReadLine(in, text))
    {
        error.result = ReplayFileResult::Truncated;
        error.line = line;
        return error;
    }
    if (!SplitLine(text, tokens, 4, count) || count != 4 || tokens[0] != "build")
    {
        error.result = ReplayFileResult::Malformed;
        error.line = line;
        return error;
    }
    const ReplayBuildIdentity build{std::string(tokens[1]), std::string(tokens[2]), std::string(tokens[3])};
    ++line;

    if (!ReadLine(in, text))
    {
        error.result = ReplayFileResult::Truncated;
        error.line = line;
        return error;
    }
    uint32_t tickCount = 0;
    if (!SplitLine(text, tokens, 3, count) || count != 2 || tokens[0] != "ticks" ||
        !ParseUInt32(tokens[1], tickCount))
    {
        error.result = ReplayFileResult::Malformed;
        error.line = line;
        return error;
    }
    ++line;

    DynamicArray<SimInput> log;
    for (uint32_t i = 0; i < tickCount; ++i)
    {
        if (!ReadLine(in, text))
        {
            error.result = ReplayFileResult::Truncated;
            error.line = line;
            return error;
        }

        SimInput input;
        if (!SplitLine(text, tokens, 4, count) || count != 4 || tokens[0] != "tick")
        {
            error.result = ReplayFileResult::Malformed;
            error.line = line;
            return error;
        }

        uint32_t axisX = 0;
        uint32_t axisY = 0;
        if (!replayfile_detail::ParseHexWord(tokens[1], input.actionBits) ||
            !replayfile_detail::ParseHexWord(tokens[2], axisX) || !replayfile_detail::ParseHexWord(tokens[3], axisY))
        {
            error.result = ReplayFileResult::Malformed;
            error.line = line;
            return error;
        }
        std::memcpy(&input.axisX, &axisX, sizeof(input.axisX));
        std::memcpy(&input.axisY, &axisY, sizeof(input.axisY));
        log.Append(input);
        ++line;
    }

    if (!ReadLine(in, text))
    {
        error.result = ReplayFileResult::Truncated;
        error.line = line;
        return error;
    }
    if (!SplitLine(text, tokens, 2, count) || count != 1 || tokens[0] != "end")
    {
        error.result = ReplayFileResult::Malformed;
        error.line = line;
        return error;
    }
    ++line;

    if (ReadLine(in, text))
    {
        error.result = ReplayFileResult::Malformed;
        error.line = line;
        return error;
    }
    if (!in.eof())
    {
        error.result = ReplayFileResult::Truncated;
        error.line = line;
        return error;
    }

    if (record.recipeVersion != CHIA_WORLD_HASH_RECIPE_VERSION)
    {
        error.result = ReplayFileResult::RecipeMismatch;
        error.fileRecipe = record.recipeVersion;
        error.binaryRecipe = CHIA_WORLD_HASH_RECIPE_VERSION;
        error.line = recipeLine;
        return error;
    }

    outRecord = record;
    outLog = log;
    outBuild = build;
    return error;
}

#endif // REPLAY_FILE_HPP
