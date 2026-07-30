#include "Test.hpp"
#include "Data/HashTable.hpp"
#include <iostream>

class HashTableTest : public Test
{
  public:
    HashTableTest(const std::string &name = "") : Test(name) {}
    bool Run() override
    {
        TEST_MESSAGE("HashTable Default Constructor");
        HashTable<int, int> ht1;
        EXPECT_TRUE(ht1.IsEmpty(), "Default HashTable should be empty.", true);
        SUCCESS_MESSAGE("HashTable Default Constructor");

        TEST_MESSAGE("HashTable Insert and Contains");
        HashTable<int, int> ht2;
        ht2.Insert(1, 100);
        ht2.Insert(2, 200);
        ht2.Insert(3, 300);
        EXPECT_TRUE(ht2.Length() == 3, "After 3 inserts length should be 3.", true);
        EXPECT_TRUE(ht2.Contains(1), "Should contain key 1.", true);
        EXPECT_TRUE(ht2.Contains(2), "Should contain key 2.", true);
        EXPECT_TRUE(ht2.Contains(3), "Should contain key 3.", true);
        EXPECT_TRUE(!ht2.Contains(99), "Should not contain key 99.", true);
        SUCCESS_MESSAGE("HashTable Insert and Contains");

        TEST_MESSAGE("HashTable Access and Modify");
        HashTable<int, int> ht3;
        ht3.Insert(1, 10);
        ht3.Insert(2, 20);
        EXPECT_TRUE(ht3[1] == 10, "ht3[1] should be 10.", true);
        EXPECT_TRUE(ht3[2] == 20, "ht3[2] should be 20.", true);
        ht3[1] = 99;
        EXPECT_TRUE(ht3[1] == 99, "After modify ht3[1] should be 99.", true);
        SUCCESS_MESSAGE("HashTable Access and Modify");

        TEST_MESSAGE("HashTable Remove");
        HashTable<int, int> ht4;
        ht4.Insert(1, 10);
        ht4.Insert(2, 20);
        ht4.Insert(3, 30);
        ht4.Remove(2);
        EXPECT_TRUE(ht4.Length() == 2, "After remove length should be 2.", true);
        EXPECT_TRUE(!ht4.Contains(2), "Should not contain removed key.", true);
        EXPECT_TRUE(ht4.Contains(1), "Should still contain key 1.", true);
        EXPECT_TRUE(ht4.Contains(3), "Should still contain key 3.", true);
        SUCCESS_MESSAGE("HashTable Remove");

        TEST_MESSAGE("HashTable Clear");
        HashTable<int, int> ht5;
        ht5.Insert(1, 10); ht5.Insert(2, 20); ht5.Insert(3, 30);
        ht5.Clear();
        EXPECT_TRUE(ht5.IsEmpty(), "After Clear should be empty.", true);
        SUCCESS_MESSAGE("HashTable Clear");

        TEST_MESSAGE("HashTable Iteration");
        HashTable<int, int> ht6;
        ht6.Insert(10, 100); ht6.Insert(20, 200); ht6.Insert(30, 300);
        int count = 0;
        for (auto itr = ht6.First(); itr != ht6.Last(); itr++)
            count++;
        EXPECT_TRUE(count == 3, "Should iterate exactly 3 entries.", true);
        SUCCESS_MESSAGE("HashTable Iteration");

        TEST_MESSAGE("HashTable Copy and Move");
        HashTable<int, int> ht7;
        ht7.Insert(1, 100); ht7.Insert(2, 200);
        HashTable<int, int> ht8(ht7);
        EXPECT_TRUE(ht8.Length() == 2, "Copy should have same length.", true);
        EXPECT_TRUE(ht8.Contains(1), "Copy should contain key 1.", true);
        HashTable<int, int> ht9(std::move(ht7));
        EXPECT_TRUE(ht9.Length() == 2, "Move should have same length.", true);
        EXPECT_TRUE(ht9[2] == 200, "Move should preserve values.", true);
        SUCCESS_MESSAGE("HashTable Copy and Move");

        TEST_MESSAGE("HashTable Value Retrieval");
        HashTable<int, int> ht10;
        ht10.Insert(1, 10); ht10.Insert(2, 20); ht10.Insert(3, 30);
        EXPECT_TRUE(ht10[1] == 10, "ht10[1] should be 10.", true);
        EXPECT_TRUE(ht10[2] == 20, "ht10[2] should be 20.", true);
        EXPECT_TRUE(ht10[3] == 30, "ht10[3] should be 30.", true);
        SUCCESS_MESSAGE("HashTable Value Retrieval");

        return true;
    }
};