#ifndef SYSTEM_TYPE_IDENTITY_TEST_HPP
#define SYSTEM_TYPE_IDENTITY_TEST_HPP

#include "Test.hpp"

#include "System/Module/TypeId.hpp"
#include "System/World/ComponentPool.hpp"
#include "System/World/World.hpp"

// namespace 隔離,避免與 SystemModule.cpp 內其它測試的全域型別衝突
// (同 ReplayTest.hpp 的做法)。見 #141。
namespace typeidentitytest
{

// 結構相同、型別不同的探針:用來證明 pool hash 只取決於內容(與池的位置),
// 不再取決於執行期的 TypeId 計數器。
struct Alpha
{
    float v;
};

struct Beta
{
    float v;
};

struct Unrelated
{
    float v;
    float w;
};

// ---------------- 測試主體 ----------------

class TypeIdentityTest : public Test
{
  public:
    TypeIdentityTest() : Test("SystemTypeIdentity")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("TypeId stays a unique functional key per type");
        EXPECT_TRUE(TypeId<Alpha>() == TypeId<Alpha>(), "TypeId<T>() is stable within a run.", true);
        EXPECT_TRUE(TypeId<Alpha>() != TypeId<Beta>(), "Different types get different ids.", true);
        EXPECT_TRUE(TypeId<Beta>() != TypeId<Unrelated>(), "Different types get different ids.", true);

        TEST_MESSAGE("pool hash is content-only, no runtime TypeId counter folded in");
        {
            ComponentPool<Alpha> alpha;
            ComponentPool<Beta> beta; // 結構相同,TypeId 不同
            alpha.Add(0, Alpha{1.5f});
            beta.Add(0, Beta{1.5f});

            EXPECT_TRUE(alpha.Hash() == beta.Hash(),
                        "Identical bytes hash equal: the pool hash no longer depends on the TypeId counter (#141).", true);

            beta.Get(0)->v = 2.5f;
            EXPECT_TRUE(alpha.Hash() != beta.Hash(), "Different values still hash differently.", true);
        }

        TEST_MESSAGE("world hash does not move when unrelated TypeIds are consumed");
        {
            World first;
            const Entity firstEntity = first.CreateEntity();
            first.AddComponent<Alpha>(firstEntity, Alpha{3.5f});
            const uint64_t baseline = first.Hash();

            // 消耗不相關的 TypeId:模擬「多註冊一個服務」或不同 attach 順序
            (void)TypeId<Unrelated>();

            World second;
            const Entity secondEntity = second.CreateEntity();
            second.AddComponent<Alpha>(secondEntity, Alpha{3.5f});

            EXPECT_TRUE(first.Hash() == baseline, "Hashing the same world twice is stable.", true);
            EXPECT_TRUE(second.Hash() == baseline, "A world built by the same steps hashes the same, whatever ids other code consumed.", true);
        }

        TEST_MESSAGE("two worlds with the same logical state hash the same, different type ids or not");
        {
            // 舊行為:即使世界的「邏輯狀態」相同,只要池的型別 id 不同(取決於誰先被請求)
            // hash 就不同 → 跨 build 比較無意義。現在池只貢獻位置 + 內容。
            World alphaWorld;
            const Entity a = alphaWorld.CreateEntity();
            alphaWorld.AddComponent<Alpha>(a, Alpha{4.5f});

            World betaWorld;
            const Entity b = betaWorld.CreateEntity();
            betaWorld.AddComponent<Beta>(b, Beta{4.5f});

            EXPECT_TRUE(alphaWorld.Hash() == betaWorld.Hash(),
                        "Same entity count, same bytes at the same pool slot -> same hash, independent of TypeId values.", true);
        }

        TEST_MESSAGE("the pool sequence is part of the fingerprint");
        {
            World withPool;
            const Entity e = withPool.CreateEntity();
            withPool.AddComponent<Alpha>(e, Alpha{1.0f});
            withPool.RemoveComponent<Alpha>(e); // 池留下來但空了
            const uint64_t hashedWithPool = withPool.Hash();

            World withoutPool;
            const Entity e2 = withoutPool.CreateEntity();
            (void)e2;

            EXPECT_TRUE(withoutPool.Hash() != hashedWithPool,
                        "A registered (empty) pool changes the hash: the pool sequence is folded in.", true);
        }

        SUCCESS_MESSAGE("SystemTypeIdentity");
        return true;
    }
};

} // namespace typeidentitytest

#endif // SYSTEM_TYPE_IDENTITY_TEST_HPP
