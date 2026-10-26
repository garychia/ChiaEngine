#ifndef SYSTEM_HASH_RECORD_TEST_HPP
#define SYSTEM_HASH_RECORD_TEST_HPP

#include "Test.hpp"

#include "Data/TypeName.hpp"
#include "System/World/World.hpp"
#include "System/World/WorldHashRecord.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

// namespace 隔離,避免與 SystemModule.cpp 內其它測試的全域型別衝突
// (同 ReplayTest.hpp 的做法)。見 #150。
namespace hashrecordtest
{

struct Alpha
{
    float v;
};

struct Beta
{
    float v;
};

struct Tag
{
};

// 稽核欄位不得為空:萃取失敗時寧可回傳整串 __PRETTY_FUNCTION__,也不該是空的。
static_assert(!TypeName<Alpha>().empty(), "TypeName must never be empty");

// ---------------- 測試主體 ----------------

class HashRecordTest : public Test
{
  public:
    HashRecordTest() : Test("SystemHashRecord")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("the record names the recipe and the registered types, in registration order");
        {
            World world;
            const Entity e = world.CreateEntity();
            world.AddComponent<Alpha>(e, Alpha{1.5f});
            world.AddComponent<Beta>(e, Beta{2.5f});
            world.AddComponent<Tag>(e, Tag{});

            WorldHashRecord record;
            world.BuildHashRecord(record);

            EXPECT_TRUE(record.recipeVersion == CHIA_WORLD_HASH_RECIPE_VERSION, "Recipe version is recorded.", true);
            EXPECT_TRUE(record.hash == world.Hash(), "Recorded hash is the world hash.", true);
            EXPECT_TRUE(record.entityCount == 1 && record.poolCount == 3, "Entity and pool counts recorded.", true);
            EXPECT_TRUE(record.pools.GetNElements() == 3, "One entry per pool.", true);

            const std::string firstName(record.pools[0].typeName);
            const std::string secondName(record.pools[1].typeName);
            const std::string thirdName(record.pools[2].typeName);
            EXPECT_TRUE(firstName.find("Alpha") != std::string::npos, "Pool 0 is Alpha (registration order).", true);
            EXPECT_TRUE(secondName.find("Beta") != std::string::npos, "Pool 1 is Beta.", true);
            EXPECT_TRUE(thirdName.find("Tag") != std::string::npos, "Pool 2 is Tag.", true);
            EXPECT_TRUE(record.pools[2].elementCount == 1, "Element counts are recorded.", true);
        }

        TEST_MESSAGE("the text form is self-describing and reproducible");
        {
            World world;
            const Entity e = world.CreateEntity();
            world.AddComponent<Alpha>(e, Alpha{1.5f});

            std::ostringstream out;
            world.WriteHashRecord(out);
            const std::string text = out.str();

            EXPECT_TRUE(text.rfind("chia-world-hash v", 0) == 0, "Header names the format and recipe version.", true);
            EXPECT_TRUE(text.find("\nhash ") != std::string::npos, "Hash line present.", true);
            EXPECT_TRUE(text.find("\nentities 1\n") != std::string::npos, "Entity count present.", true);
            EXPECT_TRUE(text.find("\npools 1\n") != std::string::npos, "Pool count present.", true);
            EXPECT_TRUE(text.find("Alpha") != std::string::npos, "Type name present.", true);

            std::ostringstream again;
            World second;
            const Entity e2 = second.CreateEntity();
            second.AddComponent<Alpha>(e2, Alpha{1.5f});
            second.WriteHashRecord(again);
            EXPECT_TRUE(again.str() == text, "Same steps produce byte-identical records.", true);
        }

        TEST_MESSAGE("the recorded type list tells apart states the digest alone cannot");
        {
            // 這條是 #147 取捨的可執行契約:型別身分不在 digest 裡,所以稽核要靠記錄。
            World alphaWorld;
            const Entity a = alphaWorld.CreateEntity();
            alphaWorld.AddComponent<Alpha>(a, Alpha{4.5f});

            World betaWorld;
            const Entity b = betaWorld.CreateEntity();
            betaWorld.AddComponent<Beta>(b, Beta{4.5f});

            WorldHashRecord alphaRecord;
            WorldHashRecord betaRecord;
            alphaWorld.BuildHashRecord(alphaRecord);
            betaWorld.BuildHashRecord(betaRecord);

            const std::string alphaName(alphaRecord.pools[0].typeName);
            const std::string betaName(betaRecord.pools[0].typeName);
            EXPECT_TRUE(alphaName.find("Alpha") != std::string::npos && betaName.find("Beta") != std::string::npos,
                        "Recorded type list separates two structurally identical component types.", true);
        }

        TEST_MESSAGE("optional record dump for CI artifacts (CHIA_HASH_RECORD_DUMP)");
        {
            const char *pPath = std::getenv("CHIA_HASH_RECORD_DUMP");
            if (pPath != nullptr && *pPath != '\0')
            {
                World world;
                const Entity e = world.CreateEntity();
                world.AddComponent<Alpha>(e, Alpha{1.5f});
                std::ofstream out(pPath);
                if (out.is_open())
                    world.WriteHashRecord(out);
            }
        }

        SUCCESS_MESSAGE("SystemHashRecord");
        return true;
    }
};

} // namespace hashrecordtest

#endif // SYSTEM_HASH_RECORD_TEST_HPP
