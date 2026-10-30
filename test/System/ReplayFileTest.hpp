#ifndef SYSTEM_REPLAY_FILE_TEST_HPP
#define SYSTEM_REPLAY_FILE_TEST_HPP

#include "Test.hpp"

#include "System/Module/ReplayFile.hpp"
#include "System/Module/SimInput.hpp"
#include "System/World/WorldHashRecord.hpp"

#include <cstdint>
#include <cstring>
#include <limits>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>

// namespace 隔離:測試標頭都併進同一個 TU(SystemModule.cpp),避免全域型別衝突。
namespace replayfiletest
{

inline uint32_t FloatBits(float value) noexcept
{
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

inline bool SameInput(const SimInput &a, const SimInput &b) noexcept
{
    return a.actionBits == b.actionBits && FloatBits(a.axisX) == FloatBits(b.axisX) &&
           FloatBits(a.axisY) == FloatBits(b.axisY);
}

inline bool SameRecord(const WorldHashRecord &a, const WorldHashRecord &b) noexcept
{
    if (a.recipeVersion != b.recipeVersion || a.hash != b.hash || a.entityCount != b.entityCount ||
        a.poolCount != b.poolCount || a.pools.GetNElements() != b.pools.GetNElements())
        return false;
    for (size_t i = 0; i < a.pools.GetNElements(); ++i)
    {
        if (a.pools[i].typeId != b.pools[i].typeId || a.pools[i].typeName != b.pools[i].typeName ||
            a.pools[i].elementCount != b.pools[i].elementCount)
            return false;
    }
    return true;
}

inline bool SameLog(const DynamicArray<SimInput> &a, const DynamicArray<SimInput> &b) noexcept
{
    if (a.GetNElements() != b.GetNElements())
        return false;
    for (size_t i = 0; i < a.GetNElements(); ++i)
    {
        if (!SameInput(a[i], b[i]))
            return false;
    }
    return true;
}

inline WorldHashRecord MakeRecord()
{
    WorldHashRecord record;
    record.recipeVersion = CHIA_WORLD_HASH_RECIPE_VERSION;
    record.hash = 0x4b1e9b7f0c9a5d31ull;
    record.entityCount = 2;
    record.poolCount = 2;

    HashRecordPool position;
    position.typeId = 1;
    position.typeName = "replaytest::Position";
    position.elementCount = 3;
    record.pools.Append(position);

    HashRecordPool velocity;
    velocity.typeId = 2;
    velocity.typeName = "replaytest::Velocity";
    velocity.elementCount = 5;
    record.pools.Append(velocity);
    return record;
}

inline WorldHashRecord SentinelRecord()
{
    WorldHashRecord record;
    record.recipeVersion = 99;
    record.hash = 0x1234;
    record.entityCount = 42;
    record.poolCount = 1;

    HashRecordPool sentinel;
    sentinel.typeId = 77;
    sentinel.typeName = "sentinel";
    sentinel.elementCount = 8;
    record.pools.Append(sentinel);
    return record;
}

inline DynamicArray<SimInput> MakeLog()
{
    DynamicArray<SimInput> log;
    log.Append(SimInput{0x80000001u, 0.1f, 1e-7f});
    log.Append(SimInput{0xFFFFFFFFu, -0.0f, std::numeric_limits<float>::denorm_min()});
    log.Append(SimInput{0x00000000u, 0.0f, -0.0f});
    log.Append(SimInput{0x7FFFFFFFu, 3.402823466e+38f, -3.402823466e+38f});
    log.Append(SimInput{0x00000002u, 1.0f, -1.0f});
    log.Append(SimInput{0x00000004u, 0.5f, 0.25f});
    log.Append(SimInput{0x00000008u, -1e-7f, 1e-7f});
    log.Append(SimInput{0x00000010u, 123456.789f, -98765.4321f});
    log.Append(SimInput{0x00000020u, 0.1f, -0.1f});
    log.Append(SimInput{0x00000040u, 1e-30f, -1e-30f});
    log.Append(SimInput{0x00000080u, 2.0f, -2.0f});
    log.Append(SimInput{0x80000000u, -0.0f, 0.0f});
    return log;
}

inline DynamicArray<SimInput> SentinelLog()
{
    DynamicArray<SimInput> log;
    log.Append(SimInput{0x12345678u, 9.0f, -9.0f});
    return log;
}

inline ReplayBuildIdentity SentinelBuild()
{
    return ReplayBuildIdentity{"sentinel-compiler", "sentinel-version", "sentinel-policy"};
}

inline bool SameBuild(const ReplayBuildIdentity &a, const ReplayBuildIdentity &b) noexcept
{
    return a.compiler == b.compiler && a.version == b.version && a.fpPolicy == b.fpPolicy;
}

inline std::string WriteText(const WorldHashRecord &record, const DynamicArray<SimInput> &log)
{
    std::ostringstream out;
    WriteReplayFile(out, record, log);
    return out.str();
}

inline std::string FindLine(const std::string &text, const char *prefix)
{
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.rfind(prefix, 0) == 0)
            return line;
    }
    return std::string();
}

// 把 CI 的 sed 投影規則在記憶體裡重現,確認 build 與 pool 的稽核欄位會被遮掉。
inline std::string ProjectReplayFile(const std::string &text)
{
    static const std::regex poolPattern(R"(^pool ([0-9]+) typeid [0-9]+ elements ([0-9]+) name .*$)",
                                        std::regex::ECMAScript);
    static const std::regex buildPattern(R"(^build .*$)", std::regex::ECMAScript);

    std::istringstream in(text);
    std::string line;
    std::string projected;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        line = std::regex_replace(line, poolPattern, "pool $1 elements $2");
        line = std::regex_replace(line, buildPattern, "build <build-identity>");
        projected += line;
        projected += '\n';
    }
    return projected;
}

// ---------------- 測試主體 ----------------

class ReplayFileTest : public Test
{
  public:
    ReplayFileTest() : Test("SystemReplayFile")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("(a) round trip preserves every field, floats included");
        {
            const WorldHashRecord record = MakeRecord();
            const DynamicArray<SimInput> log = MakeLog();
            const std::string text = WriteText(record, log);

            Check(text.find("\nticks 12\n") != std::string::npos, "the declared tick count reaches two digits");

            WorldHashRecord readRecord;
            DynamicArray<SimInput> readLog;
            ReplayBuildIdentity readBuild;
            std::istringstream in(text);
            const ReplayFileError error = ReadReplayFile(in, readRecord, readLog, readBuild);

            Check(error.result == ReplayFileResult::Ok, "the file loads");
            Check(readRecord.recipeVersion == record.recipeVersion, "recipe version survives");
            Check(readRecord.hash == record.hash, "hash survives");
            Check(readRecord.entityCount == record.entityCount, "entity count survives");
            Check(readRecord.poolCount == record.poolCount, "pool count survives");

            bool recordEqual = readRecord.pools.GetNElements() == record.pools.GetNElements();
            if (recordEqual)
            {
                for (size_t i = 0; i < record.pools.GetNElements(); ++i)
                {
                    if (readRecord.pools[i].typeId != record.pools[i].typeId ||
                        readRecord.pools[i].typeName != record.pools[i].typeName ||
                        readRecord.pools[i].elementCount != record.pools[i].elementCount)
                        recordEqual = false;
                }
            }
            Check(recordEqual, "pool entries survive");

            bool logEqual = readLog.GetNElements() == log.GetNElements();
            if (logEqual)
            {
                for (size_t i = 0; i < log.GetNElements(); ++i)
                {
                    if (!SameInput(readLog[i], log[i]))
                        logEqual = false;
                }
            }
            Check(logEqual, "every tick payload is bit-identical (including -0.0f and the subnormal)");
        }

        TEST_MESSAGE("(b) a stale recipe is rejected with both versions and the line");
        {
            const std::string text = "chia-replay v1\n"
                                     "chia-world-hash v1\n"
                                     "hash 1\n"
                                     "entities 0\n"
                                     "pools 0\n"
                                     "build gcc 13.3.0 ffp-contract=off\n"
                                     "ticks 0\n"
                                     "end\n";

            WorldHashRecord record = SentinelRecord();
            DynamicArray<SimInput> log = SentinelLog();
            ReplayBuildIdentity build = SentinelBuild();
            std::istringstream in(text);
            const ReplayFileError error = ReadReplayFile(in, record, log, build);

            Check(error.result == ReplayFileResult::RecipeMismatch, "a stale recipe is RecipeMismatch");
            Check(error.fileRecipe == 1, "fileRecipe carries the file's version");
            Check(error.binaryRecipe == CHIA_WORLD_HASH_RECIPE_VERSION, "binaryRecipe carries the binary's version");
            Check(error.line == 2, "the recipe line is reported");
            Check(SameRecord(record, SentinelRecord()), "the record output is untouched");
            Check(SameLog(log, SentinelLog()), "the log output is untouched");
            Check(SameBuild(build, SentinelBuild()), "the build output is untouched");
        }

        TEST_MESSAGE("(c) truncation is an error at the exact line, outputs untouched");
        {
            const std::string full = WriteText(MakeRecord(), MakeLog());
            std::string noEnd = full;
            if (noEnd.size() >= 4)
                noEnd.resize(noEnd.size() - 4); // drop "end\n"

            auto checkTruncated = [&](const std::string &input, uint32_t expectedLine, const char *msg) {
                WorldHashRecord record = SentinelRecord();
                DynamicArray<SimInput> log = SentinelLog();
                ReplayBuildIdentity build = SentinelBuild();
                std::istringstream in(input);
                const ReplayFileError error = ReadReplayFile(in, record, log, build);

                Check(error.result == ReplayFileResult::Truncated, msg);
                Check(error.line == expectedLine, "the truncation line is reported");
                Check(SameRecord(record, SentinelRecord()), "the record output is untouched");
                Check(SameLog(log, SentinelLog()), "the log output is untouched");
                Check(SameBuild(build, SentinelBuild()), "the build output is untouched");
            };

            checkTruncated(noEnd, 22, "a missing end line is Truncated");

            const std::string shortTrace = "chia-replay v1\n"
                                           "chia-world-hash v2\n"
                                           "hash 1\n"
                                           "entities 0\n"
                                           "pools 0\n"
                                           "build gcc 13.3.0 ffp-contract=off\n"
                                           "ticks 2\n"
                                           "tick 00000000 00000000 00000000\n";
            checkTruncated(shortTrace, 9, "fewer tick lines than declared is Truncated");

            const std::string cutRecord = "chia-replay v1\n"
                                          "chia-world-hash v2\n"
                                          "hash 1\n";
            checkTruncated(cutRecord, 4, "a stream cut inside the record block is Truncated");
        }

        TEST_MESSAGE("(d) malformed input is rejected at the exact line");
        {
            auto checkMalformed = [&](const std::string &input, uint32_t expectedLine, const char *msg) {
                WorldHashRecord record = SentinelRecord();
                DynamicArray<SimInput> log = SentinelLog();
                ReplayBuildIdentity build = SentinelBuild();
                std::istringstream in(input);
                const ReplayFileError error = ReadReplayFile(in, record, log, build);

                Check(error.result == ReplayFileResult::Malformed, msg);
                Check(error.line == expectedLine, "the malformed line is reported");
                Check(SameRecord(record, SentinelRecord()), "the record output is untouched");
                Check(SameLog(log, SentinelLog()), "the log output is untouched");
                Check(SameBuild(build, SentinelBuild()), "the build output is untouched");
            };

            checkMalformed("chia-replay v1\n"
                           "chia-world-hash v2\n"
                           "hash 1\n"
                           "entities 0\n"
                           "pools 0\n"
                           "nonsense\n"
                           "ticks 0\n"
                           "end\n",
                           6, "an unrecognized line is Malformed");

            checkMalformed("chia-replay v1\n"
                           "chia-world-hash v2\n"
                           "hash 1\n"
                           "entities 0\n"
                           "pools 0\n"
                           "build gcc 13.3.0 ffp-contract=off\n"
                           "ticks 1\n"
                           "tick 00000000 00000000 00000000\n"
                           "tick 00000001 00000000 00000000\n"
                           "end\n",
                           9, "a surplus tick line is Malformed");

            checkMalformed("chia-replay v1\n"
                           "chia-world-hash v2\n"
                           "hash 1\n"
                           "entities 0\n"
                           "pools 1\n"
                           "pool 1 typeid 5 elements 2 name Position\n"
                           "build gcc 13.3.0 ffp-contract=off\n"
                           "ticks 0\n"
                           "end\n",
                           6, "a pool ordinal not matching its position is Malformed");

            checkMalformed("chia-replay v1\n"
                           "chia-world-hash v2\n"
                           "hash 1\n"
                           "entities 0x41\n"
                           "pools 0\n"
                           "build gcc 13.3.0 ffp-contract=off\n"
                           "ticks 0\n"
                           "end\n",
                           4, "0x41 where a decimal is expected is Malformed");

            checkMalformed("chia-replay v1\n"
                           "chia-world-hash v2\n"
                           "hash 1\n"
                           "entities 0\n"
                           "pools 0\n"
                           "build gcc 13.3.0 ffp-contract=off\n"
                           "ticks 12abc\n"
                           "end\n",
                           7, "12abc where a decimal is expected is Malformed");

            checkMalformed("chia-replay v1\n"
                           "chia-world-hash v2\n"
                           "hash 1\n"
                           "entities 0\n"
                           "pools 0\n"
                           "build gcc 13.3.0 ffp-contract=off\n"
                           "ticks 1\n"
                           "tick 0000000A 00000000 00000000\n"
                           "end\n",
                           8, "an uppercase hex digit is Malformed");

            checkMalformed("chia-replay v1\n"
                           "chia-world-hash v2\n"
                           "hash 1\n"
                           "entities 0\n"
                           "pools 0\n"
                           "build gcc 13.3.0 ffp-contract=off\n"
                           "ticks 1\n"
                           "tick 00000000 00000000 00000000\n"
                           "end\n"
                           "junk\n",
                           10, "text after end is Malformed");
        }

        TEST_MESSAGE("(e) the record's text form round-trips, <unknown> included");
        {
            WorldHashRecord record;
            record.recipeVersion = CHIA_WORLD_HASH_RECIPE_VERSION;
            record.hash = 0x0123456789abcdefull;
            record.entityCount = 3;
            record.poolCount = 3;

            HashRecordPool position;
            position.typeId = 7;
            position.typeName = "replaytest::Position";
            position.elementCount = 1;
            record.pools.Append(position);

            HashRecordPool unknown;
            unknown.typeId = 8;
            unknown.typeName = "";
            unknown.elementCount = 2;
            record.pools.Append(unknown);

            HashRecordPool spaced;
            spaced.typeId = 9;
            spaced.typeName = "name with spaces";
            spaced.elementCount = 4;
            record.pools.Append(spaced);

            std::ostringstream out;
            WriteWorldHashRecord(out, record);
            const std::string text = out.str();
            Check(text.find("name <unknown>\n") != std::string::npos, "an empty name is written as <unknown>");

            std::istringstream in(text);
            WorldHashRecord readRecord;
            uint32_t line = 1;
            const HashRecordReadResult result = ReadWorldHashRecord(in, readRecord, line);

            Check(result == HashRecordReadResult::Ok, "the record block loads");
            Check(readRecord.recipeVersion == record.recipeVersion, "the record's recipe version survives");
            Check(readRecord.hash == record.hash, "the record's hash survives");
            Check(readRecord.entityCount == record.entityCount, "the record's entity count survives");
            Check(readRecord.poolCount == record.poolCount, "the record's pool count survives");
            Check(readRecord.pools.GetNElements() == record.pools.GetNElements(), "the record's pool entries survive");
            if (readRecord.pools.GetNElements() == 3)
            {
                Check(readRecord.pools[0].typeId == 7 && readRecord.pools[0].elementCount == 1 &&
                          readRecord.pools[0].typeName == "replaytest::Position",
                      "the first pool survives");
                Check(readRecord.pools[1].typeName.empty(), "<unknown> maps back to an empty name");
                Check(readRecord.pools[2].typeName == "name with spaces", "a name with spaces survives untrimmed");
            }
            Check(line == 1 + 4 + record.poolCount, "the line counter advances after every consumed line");
        }

        TEST_MESSAGE("(f) build identity round-trips, is audit-only, and is masked by the projection");
        {
            const std::string text = WriteText(MakeRecord(), MakeLog());
            const std::string buildLine = FindLine(text, "build ");
            Check(!buildLine.empty(), "the file carries a build line");

            std::string expectedBuild = "build ";
            expectedBuild += CHIA_REPLAY_BUILD_COMPILER;
            expectedBuild += " ";
            expectedBuild += CHIA_REPLAY_BUILD_VERSION;
            expectedBuild += " ";
            expectedBuild += CHIA_REPLAY_BUILD_FP_POLICY;
            Check(buildLine == expectedBuild, "the build line carries the compiler constants");

            std::string foreign = text;
            const size_t at = buildLine.empty() ? std::string::npos : foreign.find(buildLine);
            if (at != std::string::npos)
                foreign.replace(at, buildLine.size(), "build msvc 19.0 unknown");

            WorldHashRecord readRecord;
            DynamicArray<SimInput> readLog;
            ReplayBuildIdentity readBuild;
            std::istringstream ownStream(text);
            const ReplayFileError ownError = ReadReplayFile(ownStream, readRecord, readLog, readBuild);
            Check(ownError.result == ReplayFileResult::Ok, "the file loads");
            Check(readBuild.compiler == std::string(CHIA_REPLAY_BUILD_COMPILER),
                  "the compiler token reaches the caller");
            Check(readBuild.version == std::string(CHIA_REPLAY_BUILD_VERSION), "the version token reaches the caller");
            Check(readBuild.fpPolicy == std::string(CHIA_REPLAY_BUILD_FP_POLICY),
                  "the FP policy token reaches the caller");

            WorldHashRecord foreignRecord;
            DynamicArray<SimInput> foreignLog;
            ReplayBuildIdentity foreignBuild;
            std::istringstream foreignStream(foreign);
            const ReplayFileError error = ReadReplayFile(foreignStream, foreignRecord, foreignLog, foreignBuild);
            Check(error.result == ReplayFileResult::Ok, "a foreign build identity still loads (audit-only)");
            Check(foreignBuild.compiler == "msvc" && foreignBuild.version == "19.0" &&
                      foreignBuild.fpPolicy == "unknown",
                  "a foreign build identity comes back token for token");

            const std::string projected = ProjectReplayFile(text);
            Check(projected.find("build <build-identity>\n") != std::string::npos,
                  "the projection masks the build line");
            Check(projected.find("pool 0 elements 3\n") != std::string::npos,
                  "the projection masks the pool typeid and name");
            if (!buildLine.empty())
                Check(projected.find(buildLine + "\n") == std::string::npos,
                      "the projection removes the per-leg build tokens");
        }

        std::cout << "  checks: " << nPassed << " passed, " << nFailed << " failed" << std::endl;
        SUCCESS_MESSAGE("SystemReplayFile");
        return nFailed == 0;
    }

  private:
    size_t nPassed = 0;
    size_t nFailed = 0;

    bool Check(bool condition, const char *message)
    {
        EXPECT_TRUE(condition, message, false); // non-fatal: keep checking the rest of the suite
        if (condition)
        {
            nPassed++;
            std::cout << "  [PASS] " << message << std::endl;
        }
        else
        {
            nFailed++;
        }
        return true;
    }
};

} // namespace replayfiletest

#endif // SYSTEM_REPLAY_FILE_TEST_HPP
