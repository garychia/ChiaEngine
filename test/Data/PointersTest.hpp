#include "Test.hpp"
#include "Data/Pointers.hpp"
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