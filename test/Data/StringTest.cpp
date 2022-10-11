#include "StringTest.hpp"

#include "Data/String.hpp"

#include <iostream>

StringTest::StringTest(const std::string &name) : Test(name)
{
}

bool StringTest::Run()
{
    TEST_MESSAGE("String Constructors");
    char testStr[] = "Test1";
    String s1;
    EXPECT_TRUE(s1.Length() == 0, "String Empty Constructor Failed.", true);
    s1 = String(testStr, sizeof(testStr) - 1);
    EXPECT_TRUE(s1.Length() == sizeof(testStr) - 1, "String C String Constructor Failed.", true);
    SUCCESS_MESSAGE("String Constructors");

    TEST_MESSAGE("String Comparisons");
    EXPECT_TRUE(s1 == testStr, "String vs C String Comparison Failed.", true);
    SUCCESS_MESSAGE("String Comparisons");

    TEST_MESSAGE("String Find Function");
    auto idx = s1.Find('1');
    EXPECT_TRUE(idx == 4, "String Find Function Failed.", true);
    idx = s1.Find('d');
    EXPECT_TRUE(idx == -1, "String Find Function Failed.", true);
    SUCCESS_MESSAGE("String Find Function");

    TEST_MESSAGE("String SubStr Function");
    String temp = s1.SubStr(1);
    EXPECT_TRUE(temp == "est1", "String SubStr Function Failed.", true);
    temp = s1.SubStr(1, 3);
    EXPECT_TRUE(temp == "est", "String SubStr Function Failed.", true);
    SUCCESS_MESSAGE("String SubStr Function");

    TEST_MESSAGE("String Shrink Function");
    s1.Shrink(2);
    EXPECT_TRUE(s1.Length() == 2, "String Shrink Function Failed.", true);
    EXPECT_TRUE(s1 == "Te", "String Shrink Function Failed.", true);
    SUCCESS_MESSAGE("String Shrink Function");

    TEST_MESSAGE("String FromInt Function");
    s1 = String::FromInt(-9876543210);
    EXPECT_TRUE(s1 == "-9876543210", "String FromInt Function Failed.", true);
    s1 = String::FromInt(9876543210);
    EXPECT_TRUE(s1 == "9876543210", "String FromInt Function Failed.", true);
    SUCCESS_MESSAGE("String FromInt Function");

    TEST_MESSAGE("String FromFloat Function");
    s1 = String::FromFloat(-3.324252, 2);
    EXPECT_TRUE(s1 == "-3.32", "String FromFloat Function Failed.", true);
    s1 = String::FromFloat(1.234567, 4);
    EXPECT_TRUE(s1 == "1.2345", "String FromFloat Function Failed.", true);
    SUCCESS_MESSAGE("String FromFloat Function");
    return true;
}
