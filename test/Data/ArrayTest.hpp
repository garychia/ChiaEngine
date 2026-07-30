#include "Test.hpp"
#include "Data/Array.hpp"
#include <iostream>

class ArrayTest : public Test
{
  public:
    ArrayTest(const std::string &name = "") : Test(name) {}
    bool Run() override
    {
        TEST_MESSAGE("Array Default Constructor");
        Array<int> arr1;
        EXPECT_TRUE(arr1.Length() == 0, "Default Array should be empty.", true);
        SUCCESS_MESSAGE("Array Default Constructor");

        TEST_MESSAGE("Array Size Constructor");
        Array<int> arr2(5);
        EXPECT_TRUE(arr2.Length() == 5, "Array(5) should have length 5.", true);
        EXPECT_TRUE(arr2[0] == 0, "Elements should be value-initialized.", true);
        SUCCESS_MESSAGE("Array Size Constructor");

        TEST_MESSAGE("Array Initializer List");
        Array<int> arr3 = {1, 2, 3, 4, 5};
        EXPECT_TRUE(arr3.Length() == 5, "Init list should set length.", true);
        EXPECT_TRUE(arr3[0] == 1, "arr3[0] should be 1.", true);
        EXPECT_TRUE(arr3[4] == 5, "arr3[4] should be 5.", true);
        SUCCESS_MESSAGE("Array Initializer List");

        TEST_MESSAGE("Array Copy Constructor");
        Array<int> arr4(arr3);
        EXPECT_TRUE(arr4.Length() == arr3.Length(), "Copy should have same length.", true);
        EXPECT_TRUE(arr4[0] == arr3[0], "Copy should have same data.", true);
        EXPECT_TRUE(arr4[4] == arr3[4], "Copy should have same last element.", true);
        arr4[0] = 99;
        EXPECT_TRUE(arr3[0] == 1, "Modifying copy should not affect original.", true);
        SUCCESS_MESSAGE("Array Copy Constructor");

        TEST_MESSAGE("Array Move Constructor");
        Array<int> arr5({10, 20, 30});
        Array<int> arr6(std::move(arr5));
        EXPECT_TRUE(arr6.Length() == 3, "Moved-to array should have length 3.", true);
        EXPECT_TRUE(arr6[0] == 10, "Moved-to array[0] should be 10.", true);
        EXPECT_TRUE(arr6[2] == 30, "Moved-to array[2] should be 30.", true);
        EXPECT_TRUE(arr5.Length() == 0, "Moved-from array should have length 0.", true);
        SUCCESS_MESSAGE("Array Move Constructor");

        TEST_MESSAGE("Array Element + Size Constructor");
        Array<int> arr7(42, 4);
        EXPECT_TRUE(arr7.Length() == 4, "Array(42,4) should have length 4.", true);
        EXPECT_TRUE(arr7[0] == 42 && arr7[3] == 42, "All elements should be 42.", true);
        SUCCESS_MESSAGE("Array Element + Size Constructor");

        TEST_MESSAGE("Array Copy Assign");
        Array<int> arr8 = {7, 8, 9};
        Array<int> arr9;
        arr9 = arr8;
        EXPECT_TRUE(arr9.Length() == 3, "Copy assigned length should be 3.", true);
        EXPECT_TRUE(arr9[0] == 7, "Copy assigned[0] should be 7.", true);
        SUCCESS_MESSAGE("Array Copy Assign");

        TEST_MESSAGE("Array Move Assign");
        Array<int> arr10 = {100, 200};
        Array<int> arr11;
        arr11 = std::move(arr10);
        EXPECT_TRUE(arr11.Length() == 2, "Move assigned length should be 2.", true);
        EXPECT_TRUE(arr11[1] == 200, "Move assigned[1] should be 200.", true);
        SUCCESS_MESSAGE("Array Move Assign");

        TEST_MESSAGE("Array Equality");
        Array<int> arr12 = {1, 2, 3};
        Array<int> arr13 = {1, 2, 3};
        Array<int> arr14 = {1, 2, 4};
        bool eq = arr12.Length() == arr13.Length();
        for (size_t i = 0; i < arr12.Length() && eq; i++)
            eq = eq && arr12[i] == arr13[i];
        EXPECT_TRUE(eq, "Equal arrays should compare equal.", true);
        bool neq = arr12.Length() == arr14.Length();
        for (size_t i = 0; i < arr12.Length() && neq; i++)
            neq = neq && arr12[i] == arr14[i];
        EXPECT_TRUE(!neq, "Different arrays should not be equal.", true);
        SUCCESS_MESSAGE("Array Equality");

        TEST_MESSAGE("Array GetLast");
        Array<int> arr15 = {10, 20, 30};
        EXPECT_TRUE(arr15.GetLast() == 30, "GetLast should return last element.", true);
        SUCCESS_MESSAGE("Array GetLast");

        return true;
    }
};