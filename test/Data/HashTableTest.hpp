#include "Test.hpp"
#include "Data/HashTable.hpp"
#include <cstdint>
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

        TEST_MESSAGE("HashTable length across resize (regression #127)");
        {
            // The earlier cases insert 1-3 keys and never reach Expand/Shrink. This
            // one deliberately exceeds the initial bucket count to force the rebuild
            // path: a rebuild that went through the counting Insert() made Length()
            // "live elements + entries re-inserted during the rebuild", which also
            // doubled the load factor and so expanded early.
            HashTable<int, int> htBig;
            const int n = 512;
            for (int i = 0; i < n; i++)
                htBig.Insert(i, i * 10);

            EXPECT_TRUE(htBig.Length() == static_cast<size_t>(n),
                        "Length must stay equal to the number of live elements after Expand.", true);

            bool allFound = true;
            for (int i = 0; i < n; i++)
            {
                if (!htBig.Contains(i) || htBig[i] != i * 10)
                {
                    allFound = false;
                    break;
                }
            }
            EXPECT_TRUE(allFound, "Every key and value must survive the rebuild.", true);

            // Removing half drops the load factor below 0.3 and triggers a Shrink
            // rebuild; the count must not be re-added there either.
            for (int i = 0; i < n / 2; i++)
                htBig.Remove(i);
            EXPECT_TRUE(htBig.Length() == static_cast<size_t>(n / 2),
                        "Length must track removals across rebuilds (Shrink path).", true);
        }
        SUCCESS_MESSAGE("HashTable length across resize (regression #127)");

        TEST_MESSAGE("HashTable insert over an existing key (regression #137)");
        {
            // Insert() on a key that is already present must overwrite the value
            // without growing the count. ComponentPool and EntityRegistry hit this
            // on every swap-remove that moves the last element into its own slot, so
            // the drift used to compound silently: Length() stayed one too high and
            // DynamicallyResize() computed its load factor from the wrong number.
            HashTable<int, int> htDup;
            htDup.Insert(7, 70);
            htDup.Insert(7, 71);

            EXPECT_TRUE(htDup.Length() == 1,
                        "Overwriting an existing key must not increase Length.", true);
            EXPECT_TRUE(htDup[7] == 71, "Overwrite must replace the value.", true);
            EXPECT_TRUE(htDup.Contains(7), "The key must still be present.", true);

            // The compounding symptom: remove the only entry, table must be empty.
            htDup.Remove(7);
            EXPECT_TRUE(htDup.IsEmpty(),
                        "After removing the only entry the table must be empty.", true);

            // The real call-site shape: re-insert a key that is still live.
            HashTable<uint32_t, uint32_t> htSwap;
            htSwap.Insert(1u, 10u);
            htSwap.Insert(2u, 20u);
            htSwap.Insert(2u, 20u);
            EXPECT_TRUE(htSwap.Length() == 2,
                        "A redundant re-insert must not inflate Length.", true);
        }
        SUCCESS_MESSAGE("HashTable insert over an existing key (regression #137)");

        return true;
    }
};