#include "Test.hpp"
#include "Data/List.hpp"
#include <iostream>

class ListTest : public Test
{
  public:
    ListTest(const std::string &name = "") : Test(name) {}
    bool Run() override
    {
        TEST_MESSAGE("List Default Constructor");
        List<int> list1;
        EXPECT_TRUE(list1.IsEmpty(), "Default list should be empty.", true);
        EXPECT_TRUE(list1.Length() == 0, "Default list length should be 0.", true);
        SUCCESS_MESSAGE("List Default Constructor");

        TEST_MESSAGE("List Append");
        List<int> list2;
        list2.Append(10);
        list2.Append(20);
        list2.Append(30);
        EXPECT_TRUE(list2.Length() == 3, "After 3 appends length should be 3.", true);
        SUCCESS_MESSAGE("List Append");

        TEST_MESSAGE("List Iteration");
        int expected[] = {10, 20, 30};
        int idx = 0;
        for (auto itr = list2.First(); itr != list2.Last(); itr++)
        {
            EXPECT_TRUE(*itr == expected[idx], "Iteration value mismatch.", true);
            idx++;
        }
        EXPECT_TRUE(idx == 3, "Should have iterated 3 elements.", true);
        SUCCESS_MESSAGE("List Iteration");

        TEST_MESSAGE("List Prepend");
        List<int> list3;
        list3.Prepend(3);
        list3.Prepend(2);
        list3.Prepend(1);
        EXPECT_TRUE(list3.Length() == 3, "After 3 prepends length 3.", true);
        auto itr3 = list3.First();
        EXPECT_TRUE(*itr3 == 1, "First prepended should be 1.", true);
        itr3++; EXPECT_TRUE(*itr3 == 2, "Second should be 2.", true);
        itr3++; EXPECT_TRUE(*itr3 == 3, "Third should be 3.", true);
        SUCCESS_MESSAGE("List Prepend");

        TEST_MESSAGE("List Remove");
        List<int> list4;
        list4.Append(1); list4.Append(2); list4.Append(3); list4.Append(4);
        auto itr4 = list4.First();
        itr4++; // point to 2
        list4.Remove(itr4);
        EXPECT_TRUE(list4.Length() == 3, "After remove length should be 3.", true);
        auto ritr = list4.First();
        EXPECT_TRUE(*ritr == 1, "After remove first should be 1.", true);
        ritr++; EXPECT_TRUE(*ritr == 3, "After remove second should be 3.", true);
        SUCCESS_MESSAGE("List Remove");

        TEST_MESSAGE("List RemoveFirst/RemoveLast");
        List<int> list5;
        list5.Append(10); list5.Append(20); list5.Append(30);
        list5.RemoveFirst();
        EXPECT_TRUE(list5.Length() == 2, "After RemoveFirst length 2.", true);
        auto itr5 = list5.First();
        EXPECT_TRUE(*itr5 == 20, "After RemoveFirst first should be 20.", true);
        list5.RemoveLast();
        EXPECT_TRUE(list5.Length() == 1, "After RemoveLast length 1.", true);
        EXPECT_TRUE(*(list5.First()) == 20, "Only remaining should be 20.", true);
        SUCCESS_MESSAGE("List RemoveFirst/RemoveLast");

        TEST_MESSAGE("List Contains and Find");
        List<int> list6;
        list6.Append(5); list6.Append(10); list6.Append(15);
        EXPECT_TRUE(list6.Contains(10), "List should contain 10.", true);
        EXPECT_TRUE(!list6.Contains(99), "List should not contain 99.", true);
        auto found = list6.Find(15);
        EXPECT_TRUE(*found == 15, "Find(15) should point to 15.", true);
        SUCCESS_MESSAGE("List Contains and Find");

        TEST_MESSAGE("List RemoveAll");
        List<int> list7;
        list7.Append(1); list7.Append(2); list7.Append(3);
        list7.RemoveAll();
        EXPECT_TRUE(list7.IsEmpty(), "After RemoveAll should be empty.", true);
        EXPECT_TRUE(list7.Length() == 0, "Length should be 0.", true);
        // Should still be usable
        list7.Append(100);
        EXPECT_TRUE(list7.Length() == 1, "Should accept appends after RemoveAll.", true);
        SUCCESS_MESSAGE("List RemoveAll");

        TEST_MESSAGE("List Insert");
        List<int> list8;
        list8.Append(1); list8.Append(3);
        auto itr8 = list8.First();
        itr8++; // point to 3
        list8.Insert(2, itr8);
        EXPECT_TRUE(list8.Length() == 3, "After insert length should be 3.", true);
        auto i8 = list8.First();
        EXPECT_TRUE(*i8 == 1, "First should be 1.", true);
        i8++; EXPECT_TRUE(*i8 == 2, "Second should be 2 (inserted).", true);
        i8++; EXPECT_TRUE(*i8 == 3, "Third should be 3.", true);
        SUCCESS_MESSAGE("List Insert");

        return true;
    }
};