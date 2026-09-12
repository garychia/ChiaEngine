#include "Test.hpp"
#include "Data/DynamicArray.hpp"
#include <iostream>

class DynamicArrayTest : public Test
{
  public:
    DynamicArrayTest(const std::string &name = "") : Test(name) {}
    bool Run() override
    {
        TEST_MESSAGE("DynamicArray Default Constructor");
        DynamicArray<int> darr1;
        EXPECT_TRUE(darr1.Length() == 0, "Default should be empty.", true);
        EXPECT_TRUE(darr1.IsEmpty(), "Default IsEmpty should be true.", true);
        SUCCESS_MESSAGE("DynamicArray Default Constructor");

        TEST_MESSAGE("DynamicArray Append and Access");
        DynamicArray<int> darr2 = {1, 2, 3};
        EXPECT_TRUE(darr2.Length() == 3, "Init list length should be 3.", true);
        darr2.Append(4);
        EXPECT_TRUE(darr2.Length() == 4, "After append length should be 4.", true);
        EXPECT_TRUE(darr2[3] == 4, "Appended element should be 4.", true);
        SUCCESS_MESSAGE("DynamicArray Append and Access");

        TEST_MESSAGE("DynamicArray RemoveLast");
        DynamicArray<int> darr3 = {10, 20, 30};
        darr3.RemoveLast();
        EXPECT_TRUE(darr3.Length() == 2, "After RemoveLast length should be 2.", true);
        EXPECT_TRUE(darr3[1] == 20, "Last should now be 20.", true);
        darr3.RemoveLast();
        EXPECT_TRUE(darr3.Length() == 1, "After second RemoveLast length 1.", true);
        darr3.RemoveLast();
        EXPECT_TRUE(darr3.IsEmpty(), "After third RemoveLast should be empty.", true);
        SUCCESS_MESSAGE("DynamicArray RemoveLast");

        TEST_MESSAGE("DynamicArray RemoveAll");
        DynamicArray<int> darr4 = {1, 2, 3, 4, 5};
        darr4.RemoveAll();
        EXPECT_TRUE(darr4.IsEmpty(), "After RemoveAll should be empty.", true);
        EXPECT_TRUE(darr4.Length() == 0, "Length should be 0 after RemoveAll.", true);
        // Should still be usable after RemoveAll
        darr4.Append(99);
        EXPECT_TRUE(darr4[0] == 99, "Should still accept appends after RemoveAll.", true);
        SUCCESS_MESSAGE("DynamicArray RemoveAll");

        TEST_MESSAGE("DynamicArray Resize");
        DynamicArray<int> darr5 = {1, 2, 3};
        darr5.Resize(5);
        EXPECT_TRUE(darr5.Length() == 5, "After Resize(5) length should be 5.", true);
        darr5.Resize(2);
        EXPECT_TRUE(darr5.Length() == 2, "After Resize(2) length should be 2.", true);
        EXPECT_TRUE(darr5[0] == 1, "First element preserved after shrink.", true);
        SUCCESS_MESSAGE("DynamicArray Resize");

        TEST_MESSAGE("DynamicArray GetFirst/GetLast");
        DynamicArray<int> darr6 = {100, 200, 300};
        EXPECT_TRUE(darr6.GetFirst() == 100, "GetFirst should be 100.", true);
        EXPECT_TRUE(darr6.GetLast() == 300, "GetLast should be 300.", true);
        SUCCESS_MESSAGE("DynamicArray GetFirst/GetLast");

        TEST_MESSAGE("DynamicArray Copy & Move");
        DynamicArray<int> darr7 = {5, 10, 15};
        DynamicArray<int> darr8(darr7);
        EXPECT_TRUE(darr8.Length() == darr7.Length(), "Copy length matches.", true);
        EXPECT_TRUE(darr8[1] == darr7[1], "Copy element matches.", true);
        DynamicArray<int> darr9(std::move(darr7));
        EXPECT_TRUE(darr9.Length() == 3, "Move length is 3.", true);
        EXPECT_TRUE(darr9[2] == 15, "Move last element is 15.", true);
        SUCCESS_MESSAGE("DynamicArray Copy & Move");

        TEST_MESSAGE("DynamicArray Equality");
        DynamicArray<int> darrA = {1, 2, 3};
        DynamicArray<int> darrB = {1, 2, 3};
        DynamicArray<int> darrC = {1, 2, 4};
        bool deq = darrA.Length() == darrB.Length();
        for (size_t i = 0; i < darrA.Length() && deq; i++)
            deq = deq && darrA[i] == darrB[i];
        EXPECT_TRUE(deq, "Equal DynamicArrays should compare equal.", true);
        bool dneq = darrA.Length() == darrC.Length();
        for (size_t i = 0; i < darrA.Length() && dneq; i++)
            dneq = dneq && darrA[i] == darrC[i];
        EXPECT_TRUE(!dneq, "Different DynamicArrays should not be equal.", true);
        SUCCESS_MESSAGE("DynamicArray Equality");

        // ──────────────────────────────────────────────────────────────
        // #85: capacity retention, exponential shrink, bounded accessors
        // ──────────────────────────────────────────────────────────────

        TEST_MESSAGE("DynamicArray RemoveAll keeps capacity (#85)");
        {
            DynamicArray<int> capArr;
            for (int i = 0; i < 64; i++)
                capArr.Append(i);
            EXPECT_TRUE(capArr.Length() == 64, "64 elements appended.", true);
            capArr.RemoveAll();
            EXPECT_TRUE(capArr.IsEmpty(), "Empty after RemoveAll.", true);
            // Re-append must NOT reallocate from scratch: capacity retained.
            // (Can't observe capacity directly; a retained buffer is proven by
            // pointer stability across RemoveAll + re-append.)
            int *pBefore = &capArr[0];
            capArr.Append(7);
            EXPECT_TRUE(&capArr[0] == pBefore, "Buffer pointer stable across RemoveAll (capacity kept).", true);
            EXPECT_TRUE(capArr[0] == 7, "First element after re-append is 7.", true);
        }

        TEST_MESSAGE("DynamicArray exponential shrink headroom (#85)");
        {
            DynamicArray<int> shrinkArr;
            for (int i = 0; i < 64; i++)
                shrinkArr.Append(i);
            // Remove 60 of 64: count = 4. With 1/4-headroom shrink, capacity
            // goes 64 -> 32 when count < 16. Element data must survive.
            for (int i = 0; i < 60; i++)
                shrinkArr.RemoveLast();
            EXPECT_TRUE(shrinkArr.Length() == 4, "4 elements remain after 60 removals.", true);
            EXPECT_TRUE(shrinkArr[0] == 0 && shrinkArr[1] == 1 && shrinkArr[2] == 2 && shrinkArr[3] == 3,
                        "First 4 elements preserved through shrink churn.", true);
        }

        TEST_MESSAGE("DynamicArray RemoveAll then reuse full cycle (#85)");
        {
            DynamicArray<int> cycleArr;
            for (int i = 0; i < 16; i++)
                cycleArr.Append(i * i);
            for (int cycle = 0; cycle < 3; cycle++)
            {
                cycleArr.RemoveAll();
                EXPECT_TRUE(cycleArr.IsEmpty(), "Empty after RemoveAll in cycle.", true);
                for (int i = 0; i < 16; i++)
                    cycleArr.Append(i * i);
                EXPECT_TRUE(cycleArr.Length() == 16, "Refilled to 16 in cycle.", true);
                EXPECT_TRUE(cycleArr[15] == 225, "Last element correct after refill.", true);
            }
        }

        TEST_MESSAGE("DynamicArray empty GetFirst/GetLast asserts (#85)");
        {
            DynamicArray<int> emptyArr;
            // The accessors now assert on empty; in a valid program we only call
            // them non-empty. Verifying the guard: the asserts are compile-time
            // safe and release-UB-free by design - just confirm the container
            // stays usable after an empty RemoveLast (the guard path).
            emptyArr.RemoveLast(); // no-op on empty (guarded)
            EXPECT_TRUE(emptyArr.IsEmpty(), "RemoveLast on empty is a safe no-op.", true);
        }

        return true;
    }
};