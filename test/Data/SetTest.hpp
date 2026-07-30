#include "Test.hpp"
#include "Data/Set.hpp"
#include <iostream>

class SetTest : public Test
{
  public:
    SetTest(const std::string &name = "") : Test(name) {}
    bool Run() override
    {
        TEST_MESSAGE("Set Default Constructor");
        Set<int> set1;
        EXPECT_TRUE(set1.IsEmpty(), "Default set should be empty.", true);
        EXPECT_TRUE(set1.Length() == 0, "Length should be 0.", true);
        SUCCESS_MESSAGE("Set Default Constructor");

        TEST_MESSAGE("Set Insert and Contains");
        Set<int> set2;
        set2.Insert(10);
        set2.Insert(20);
        set2.Insert(30);
        EXPECT_TRUE(set2.Length() == 3, "After 3 inserts length should be 3.", true);
        EXPECT_TRUE(set2.Contains(10), "Set should contain 10.", true);
        EXPECT_TRUE(set2.Contains(20), "Set should contain 20.", true);
        EXPECT_TRUE(set2.Contains(30), "Set should contain 30.", true);
        EXPECT_TRUE(!set2.Contains(99), "Set should not contain 99.", true);
        SUCCESS_MESSAGE("Set Insert and Contains");

        TEST_MESSAGE("Set Duplicate Insert");
        Set<int> set3;
        set3.Insert(5);
        set3.Insert(5);
        set3.Insert(5);
        EXPECT_TRUE(set3.Length() == 1, "Duplicate inserts should not increase length.", true);
        SUCCESS_MESSAGE("Set Duplicate Insert");

        TEST_MESSAGE("Set Remove");
        Set<int> set4 = {1, 2, 3, 4, 5};
        EXPECT_TRUE(set4.Length() == 5, "Init list should have 5 elements.", true);
        set4.Remove(3);
        EXPECT_TRUE(set4.Length() == 4, "After remove length should be 4.", true);
        EXPECT_TRUE(!set4.Contains(3), "Set should not contain removed element.", true);
        set4.Remove(1);
        EXPECT_TRUE(!set4.Contains(1), "Set should not contain removed first element.", true);
        SUCCESS_MESSAGE("Set Remove");

        TEST_MESSAGE("Set Clear");
        Set<int> set5 = {10, 20, 30, 40};
        set5.Clear();
        EXPECT_TRUE(set5.IsEmpty(), "After Clear should be empty.", true);
        EXPECT_TRUE(set5.Length() == 0, "After Clear length 0.", true);
        SUCCESS_MESSAGE("Set Clear");

        TEST_MESSAGE("Set Iteration");
        Set<int> set6 = {100, 200, 300};
        int count = 0;
        int sum = 0;
        for (auto itr = set6.First(); itr != set6.Last(); itr++)
        {
            count++;
            sum += *itr;
        }
        EXPECT_TRUE(count == 3, "Should iterate exactly 3 elements.", true);
        EXPECT_TRUE(sum == 600, "Sum should be 600.", true);
        SUCCESS_MESSAGE("Set Iteration");

        TEST_MESSAGE("Set Copy and Move");
        Set<int> set7 = {1, 2, 3};
        Set<int> set8(set7);
        EXPECT_TRUE(set8.Length() == 3, "Copy should have same length.", true);
        EXPECT_TRUE(set8.Contains(2), "Copy should contain copied elements.", true);
        Set<int> set9(std::move(set7));
        EXPECT_TRUE(set9.Length() == 3, "Move should have same length.", true);
        EXPECT_TRUE(set9.Contains(1), "Move should contain moved elements.", true);
        SUCCESS_MESSAGE("Set Copy and Move");

        return true;
    }
};