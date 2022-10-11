#include "IOTest.hpp"
#include "System/IO/IO.hpp"

IOTest::IOTest(const String &testPath) : Test("IOTest"), testPath(testPath)
{
}

bool IOTest::Run()
{
    TEST_MESSAGE("FileIO ReadBytes Function");
    FileIO file(testPath + String("test1.txt"));
    EXPECT_TRUE(file.Open(), "FileIO failed to open the test file.", true);
    EXPECT_TRUE(file.IsOpened(), "FileIO::IsOpened should have returned true.", false);
    char buffer[3];
    auto nBytesRead = file.ReadBytes(3, (char *)buffer);
    file.Close();
    EXPECT_TRUE(nBytesRead == 3, "FileIO::ReadBytes should have returned 3.", true);
    EXPECT_TRUE(!file.IsOpened(), "FileIO::IsOpened should have returned false.", false);
    for (int i = 0; i < 3; i++)
        EXPECT_TRUE(buffer[i] == '1' + i, "FileIO ReadBytes Function Failed.", true);
    SUCCESS_MESSAGE("FileIO ReadBytes Function");

    TEST_MESSAGE("FileIO ReadLine Function");
    file.Open();
    auto line1 = file.ReadLine<char>();
    auto line2 = file.ReadLine<char>();
    file.Close();
    EXPECT_TRUE(line1 == "123", "FileIO::ReadLine should have returned \"123\".", true);
    EXPECT_TRUE(line2 == "", "FileIO::ReadLine should have returned \"\".", true);
    SUCCESS_MESSAGE("FileIO ReadLine Function");
    return true;
}
