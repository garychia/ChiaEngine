#ifndef SYSTEM_PADDING_AUDIT_TEST_HPP
#define SYSTEM_PADDING_AUDIT_TEST_HPP

#include "Test.hpp"

#include "Data/Padding.hpp"
#include "Geometry/3D/Point3D.hpp"
#include "Physics/ColliderComponent.hpp"
#include "System/Module/SimInput.hpp"
#include "System/Pong/PongTypes.hpp"
#include "System/World/ComponentPool.hpp"

#include <cstring>
#include <new>

// namespace 隔離,避免與 SystemModule.cpp 內其它測試的全域型別衝突
// (同 ReplayTest.hpp 的做法)。見 #142。
namespace paddingaudittest
{

// ---------------- 探針型別:先證明 guard 有牙齒 ----------------

struct DenseProbe
{
    float a;
    float b;
};

struct PaddedProbe
{
    float a;
    uint8_t b; // 之後有 ABI 的 3 byte 隱含 padding
};

struct EmptyProbe
{
};

CHIA_PADDING_FREE(DenseProbe, &DenseProbe::a, &DenseProbe::b);
static_assert(!::NoPadding<PaddedProbe>(&PaddedProbe::a, &PaddedProbe::b),
              "PaddedProbe must be caught as padded: the guard has to have teeth.");
static_assert(::NoPadding<EmptyProbe>(), "An empty type has no padding bytes.");

// #148:尺寸守恆對「成員型別內部的 padding」是盲的 —— sizeof(成員) 已經含了那份
// padding,等號兩邊一起變大。所以成員型別必須自己帶 CHIA_PADDING_FREE 的標記。
struct NestedDenseProbe
{
    DenseProbe inner; // 已標記 + 尺寸守恆 → 可以證明
};

struct UnmarkedProbe
{
    float a; // 沒有標記:就算它自己沒 padding,也沒人證明過
};

struct HoldsUnmarkedProbe
{
    UnmarkedProbe inner;
};

union MismatchedPayload
{
    float f;
    double d;
};

struct HoldsUnionProbe
{
    MismatchedPayload payload; // 寫了 float 之後 double 的那 4 byte 是 unspecified
};

CHIA_PADDING_FREE(NestedDenseProbe, &NestedDenseProbe::inner);
static_assert(!::NoPadding<HoldsUnmarkedProbe>(&HoldsUnmarkedProbe::inner),
              "An unmarked nested member type must not pass (#148).");
static_assert(!::NoPadding<HoldsUnionProbe>(&HoldsUnionProbe::payload),
              "A size-mismatched union member must not pass (#148).");

// ---------------- 工具:造出「成員有值、但儲存空間先被垃圾填過」的元件 ----------------

// 這正是 #129 的失敗場景:trivially copyable 的複製/指派允許整塊複製,來源物件的
// padding 垃圾會一起進到 ComponentPool,而 Hash() 逐位元組讀它。先在 buffer 裡填垃圾
// (那些 byte 因此是「已定義」的),再整塊複製 —— 成員有值,不該存在的 byte 帶垃圾。
template <class T> T PoisonedCopy(const T &value, uint8_t fill)
{
    alignas(T) unsigned char storage[sizeof(T)];
    std::memset(storage, fill, sizeof(T));
    T *pPlaced = new (storage) T(value);
    T copy = *pPlaced;
    return copy;
}

// ---------------- 測試主體 ----------------

class PaddingAuditTest : public Test
{
  public:
    PaddingAuditTest() : Test("SystemPaddingAudit")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("guard rejects padded types, accepts dense and empty ones");
        EXPECT_TRUE(::NoPadding<DenseProbe>(&DenseProbe::a, &DenseProbe::b), "DenseProbe is padding-free.", true);
        EXPECT_TRUE(!::NoPadding<PaddedProbe>(&PaddedProbe::a, &PaddedProbe::b), "PaddedProbe is detected.", true);
        EXPECT_TRUE(::NoPadding<EmptyProbe>(), "EmptyProbe has no padding bytes.", true);
        EXPECT_TRUE(::NoPadding<NestedDenseProbe>(&NestedDenseProbe::inner),
                    "A marked, size-conserving nested member type passes (#148).", true);
        EXPECT_TRUE(!::NoPadding<HoldsUnmarkedProbe>(&HoldsUnmarkedProbe::inner),
                    "An unmarked nested member type is rejected: sizeof() would hide its padding (#148).", true);
        EXPECT_TRUE(!::NoPadding<HoldsUnionProbe>(&HoldsUnionProbe::payload),
                    "A size-mismatched union member is rejected (#148).", true);

        TEST_MESSAGE("every sim component is padding-free");
        EXPECT_TRUE(::NoPadding<Point3D>(&Point3D::x, &Point3D::y, &Point3D::z), "Point3D: 3 members = sizeof.",
                    true);
        EXPECT_TRUE(::NoPadding<SimInput>(&SimInput::actionBits, &SimInput::axisX, &SimInput::axisY),
                    "SimInput: 3 members = sizeof.", true);
        EXPECT_TRUE(::NoPadding<ColliderComponent>(&ColliderComponent::kind, &ColliderComponent::data),
                    "ColliderComponent: 32 = 32 after widening Kind.", true);
        EXPECT_TRUE(::NoPadding<pong::BallComponent>(&pong::BallComponent::position, &pong::BallComponent::velocity),
                    "BallComponent.", true);
        EXPECT_TRUE(::NoPadding<pong::PaddleComponent>(&pong::PaddleComponent::position,
                                                       &pong::PaddleComponent::halfHeight,
                                                       &pong::PaddleComponent::speed),
                    "PaddleComponent.", true);
        EXPECT_TRUE(::NoPadding<pong::OptionalPaddlePlayer>(&pong::OptionalPaddlePlayer::player,
                                                            &pong::OptionalPaddlePlayer::isCpu),
                    "OptionalPaddlePlayer.", true);

        TEST_MESSAGE("poisoned storage cannot change a dense type's pool hash");
        {
            DenseProbe value;
            value.a = 1.5f;
            value.b = -2.5f;

            ComponentPool<DenseProbe> clean;
            ComponentPool<DenseProbe> poisoned;
            clean.Add(0, value);
            poisoned.Add(0, PoisonedCopy(value, 0xAA));

            EXPECT_TRUE(clean.Hash() == poisoned.Hash(),
                        "Two pools with identical dense values hash equal, whatever the storage held before.", true);
        }

        TEST_MESSAGE("an empty tag component hashes deterministically");
        {
            ComponentPool<EmptyProbe> first;
            ComponentPool<EmptyProbe> second;
            first.Add(0, PoisonedCopy(EmptyProbe{}, 0xAA));
            second.Add(0, PoisonedCopy(EmptyProbe{}, 0x55));

            EXPECT_TRUE(first.GetNElements() == 1 && second.GetNElements() == 1, "Both empty components are stored.",
                        true);
            EXPECT_TRUE(first.Hash() == second.Hash(),
                        "Empty components hash equal: Hash() does not read their unspecified storage byte.", true);

            // 更強的版本:直接把池中那顆 byte 塗成垃圾,pool hash 仍不得改變。
            const uint64_t before = first.Hash();
            std::memset(static_cast<void *>(first.Get(0)), 0xAA, sizeof(EmptyProbe));
            EXPECT_TRUE(first.Hash() == before,
                        "Rewriting an empty component's storage byte leaves the pool hash unchanged.", true);
        }

        SUCCESS_MESSAGE("SystemPaddingAudit");
        return true;
    }
};

} // namespace paddingaudittest

#endif // SYSTEM_PADDING_AUDIT_TEST_HPP
