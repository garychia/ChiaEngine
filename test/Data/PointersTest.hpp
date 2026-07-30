#include "Test.hpp"
#include "Data/Pointers.hpp"
#include "Data/Maybe.hpp"
#include "Data/Pair.hpp"
#include <iostream>

class PointersTest : public Test
{
  public:
    PointersTest(const std::string &name = "") : Test(name) {}
    bool Run() override
    {
        TEST_MESSAGE("Ptr Default");
        Ptr<int> p1;
        EXPECT_TRUE(!p1.IsValid(), "Default Ptr should be invalid.", true);
        SUCCESS_MESSAGE("Ptr Default");

        TEST_MESSAGE("Ptr Construct");
        Ptr<int> p2 = Ptr<int>::Construct(42);
        EXPECT_TRUE(p2.IsValid(), "Constructed Ptr should be valid.", true);
        EXPECT_TRUE(*p2 == 42, "Dereferenced value should be 42.", true);
        SUCCESS_MESSAGE("Ptr Construct");

        TEST_MESSAGE("Ptr Release");
        Ptr<int> p3 = Ptr<int>::Construct(99);
        EXPECT_TRUE(p3.IsValid(), "Should be valid before release.", true);
        p3.Release();
        EXPECT_TRUE(!p3.IsValid(), "Should be invalid after release.", true);
        SUCCESS_MESSAGE("Ptr Release");

        TEST_MESSAGE("SharedPtr Construct");
        SharedPtr<int> sp1 = SharedPtr<int>::Construct(100);
        EXPECT_TRUE(sp1.IsValid(), "SharedPtr should be valid.", true);
        EXPECT_TRUE(*sp1 == 100, "Dereferenced value should be 100.", true);
        SUCCESS_MESSAGE("SharedPtr Construct");

        TEST_MESSAGE("SharedPtr Copy");
        SharedPtr<int> sp2 = SharedPtr<int>::Construct(200);
        SharedPtr<int> sp3(sp2);
        EXPECT_TRUE(sp3.IsValid(), "Copied SharedPtr should be valid.", true);
        EXPECT_TRUE(*sp3 == 200, "Copied value should be 200.", true);
        SUCCESS_MESSAGE("SharedPtr Copy");

        TEST_MESSAGE("SharedPtr Release");
        SharedPtr<int> sp4 = SharedPtr<int>::Construct(300);
        sp4.Release();
        EXPECT_TRUE(!sp4.IsValid(), "Released SharedPtr should be invalid.", true);
        SUCCESS_MESSAGE("SharedPtr Release");

        TEST_MESSAGE("WeakPtr from SharedPtr");
        SharedPtr<int> sp5 = SharedPtr<int>::Construct(400);
        WeakPtr<int> wp1(sp5);
        EXPECT_TRUE(wp1.IsValid(), "WeakPtr should be valid while SharedPtr exists.", true);
        SUCCESS_MESSAGE("WeakPtr from SharedPtr");

        TEST_MESSAGE("WeakPtr operator->");
        SharedPtr<int> sp6 = SharedPtr<int>::Construct(500);
        WeakPtr<int> wp2(sp6);
        // Just verify it doesn't crash and returns a valid pointer
        int *raw = wp2.operator->();
        EXPECT_TRUE(raw != nullptr, "operator-> should return non-null when valid.", true);
        SUCCESS_MESSAGE("WeakPtr operator->");

        // ===== Maybe Tests =====
        TEST_MESSAGE("Maybe Default");
        Maybe<int> m1;
        EXPECT_TRUE(!m1.IsValid(), "Default Maybe should be invalid.", true);
        EXPECT_TRUE(!m1, "Bool operator should be false.", true);
        SUCCESS_MESSAGE("Maybe Default");

        TEST_MESSAGE("Maybe Value");
        Maybe<int> m2(42);
        EXPECT_TRUE(m2.IsValid(), "Value Maybe should be valid.", true);
        EXPECT_TRUE(m2.Get() == 42, "Get() should return 42.", true);
        EXPECT_TRUE((bool)m2, "Bool operator should be true.", true);
        SUCCESS_MESSAGE("Maybe Value");

        TEST_MESSAGE("Maybe Assign");
        Maybe<int> m3;
        m3 = 99;
        EXPECT_TRUE(m3.IsValid(), "After assign should be valid.", true);
        EXPECT_TRUE(m3.Get() == 99, "After assign value should be 99.", true);
        SUCCESS_MESSAGE("Maybe Assign");

        TEST_MESSAGE("Maybe Remove");
        Maybe<int> m4(77);
        m4.Remove();
        EXPECT_TRUE(!m4.IsValid(), "After Remove should be invalid.", true);
        SUCCESS_MESSAGE("Maybe Remove");

        TEST_MESSAGE("Maybe Copy");
        Maybe<int> m5(55);
        Maybe<int> m6(m5);
        EXPECT_TRUE(m6.IsValid(), "Copied Maybe should be valid.", true);
        EXPECT_TRUE(m6.Get() == 55, "Copied value should be 55.", true);
        SUCCESS_MESSAGE("Maybe Copy");

        TEST_MESSAGE("Maybe Equality");
        Maybe<int> m7(10);
        Maybe<int> m8(10);
        Maybe<int> m9(20);
        EXPECT_TRUE(m7 == m8, "Equal Maybes should compare equal.", true);
        EXPECT_TRUE(!(m7 == m9), "Different Maybes should not compare equal.", true);
        SUCCESS_MESSAGE("Maybe Equality");

        // ===== Pair Tests =====
        TEST_MESSAGE("Pair Key and Value");
        Pair<int, int> pair1;
        pair1.Key() = 1;
        pair1.Value() = 100;
        EXPECT_TRUE(pair1.Key() == 1, "Key should be 1.", true);
        EXPECT_TRUE(pair1.Value() == 100, "Value should be 100.", true);
        SUCCESS_MESSAGE("Pair Key and Value");

        TEST_MESSAGE("Pair Copy Assign");
        Pair<int, int> pair2;
        pair2 = pair1;
        EXPECT_TRUE(pair2.Key() == 1, "Copied key should be 1.", true);
        EXPECT_TRUE(pair2.Value() == 100, "Copied value should be 100.", true);
        SUCCESS_MESSAGE("Pair Copy Assign");

        return true;
    }
};