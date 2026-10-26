#ifndef SYSTEM_ENTITY_IDENTITY_TEST_HPP
#define SYSTEM_ENTITY_IDENTITY_TEST_HPP

#include "Test.hpp"

#include "System/World/World.hpp"

// namespace 隔離,避免與 SystemModule.cpp 內其它測試的全域型別衝突
// (同 ReplayTest.hpp 的做法)。見 #149。
namespace entityidentitytest
{

struct Alpha
{
    float v;
};

// ---------------- 測試主體 ----------------

class EntityIdentityTest : public Test
{
  public:
    EntityIdentityTest() : Test("SystemEntityIdentity")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("recycle history is part of the world hash (generations + dense order)");
        {
            World fresh; // slot 0 never recycled
            const Entity freshEntity = fresh.CreateEntity();
            fresh.AddComponent<Alpha>(freshEntity, Alpha{1.0f});

            World recycled; // slot 0 created, destroyed, re-created -> generation +1
            const Entity dead = recycled.CreateEntity();
            recycled.DestroyEntity(dead);
            const Entity recycledEntity = recycled.CreateEntity();
            recycled.AddComponent<Alpha>(recycledEntity, Alpha{1.0f});

            EXPECT_TRUE(recycledEntity.GetIndex() == freshEntity.GetIndex(), "Both take slot 0.", true);
            EXPECT_TRUE(recycledEntity.GetGeneration() != freshEntity.GetGeneration(), "Generations differ.", true);
            EXPECT_TRUE(fresh.Hash() != recycled.Hash(),
                        "Same component bytes in the same slot with a different generation must hash differently (#149).",
                        true);
        }

        TEST_MESSAGE("identical construction steps still hash identically");
        {
            World first;
            const Entity a = first.CreateEntity();
            first.AddComponent<Alpha>(a, Alpha{2.0f});

            World second;
            const Entity b = second.CreateEntity();
            second.AddComponent<Alpha>(b, Alpha{2.0f});

            EXPECT_TRUE(first.Hash() == second.Hash(), "Same steps, same hash.", true);
        }

        TEST_MESSAGE("destroy and re-create does not restore the old fingerprint");
        {
            World world;
            const Entity a = world.CreateEntity();
            const Entity b = world.CreateEntity();
            world.AddComponent<Alpha>(a, Alpha{1.0f});
            world.AddComponent<Alpha>(b, Alpha{2.0f});
            const uint64_t before = world.Hash();

            world.DestroyEntity(b);
            EXPECT_TRUE(world.Hash() != before, "Destroying an entity changes the hash.", true);

            const Entity c = world.CreateEntity(); // same slot as b, next generation
            world.AddComponent<Alpha>(c, Alpha{2.0f});
            EXPECT_TRUE(c.GetIndex() == b.GetIndex(), "The new entity reuses the freed slot.", true);
            EXPECT_TRUE(world.Hash() != before,
                        "Re-created into the same slot with the same value, the state is still different (#149).", true);
        }

        TEST_MESSAGE("dense walk order is part of the fingerprint");
        {
            World ordered;
            const Entity a = ordered.CreateEntity();
            const Entity b = ordered.CreateEntity();
            ordered.AddComponent<Alpha>(a, Alpha{1.0f});
            ordered.AddComponent<Alpha>(b, Alpha{2.0f});

            World reshuffled; // swap-remove reorders dense, then a new entity fills the hole
            const Entity c = reshuffled.CreateEntity();
            const Entity d = reshuffled.CreateEntity();
            reshuffled.AddComponent<Alpha>(c, Alpha{1.0f});
            reshuffled.AddComponent<Alpha>(d, Alpha{2.0f});
            reshuffled.DestroyEntity(c);
            const Entity e = reshuffled.CreateEntity();
            reshuffled.AddComponent<Alpha>(e, Alpha{1.0f});

            EXPECT_TRUE(ordered.Hash() != reshuffled.Hash(),
                        "Different entity layout and walk order hashes differently.", true);
        }

        SUCCESS_MESSAGE("SystemEntityIdentity");
        return true;
    }
};

} // namespace entityidentitytest

#endif // SYSTEM_ENTITY_IDENTITY_TEST_HPP
