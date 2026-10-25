#ifndef TEST_HPP
#define TEST_HPP

#include <cstdlib>
#include <deque>
#include <iostream>
#include <string>


#define PRINT_ERR(msg)                                                                                                 \
    std::cerr << msg << std::endl                                                                                      \
              << "In file " << __FILE__ << ", "                                                                        \
              << "line " << __LINE__ << "." << std::endl
#define EXPECT_TRUE(cond, msg, return_false)                                                                           \
    if (!(cond))                                                                                                       \
    {                                                                                                                  \
        std::cerr << "Expected '" #cond "' to be true." << std::endl;                                                  \
        PRINT_ERR(msg);                                                                                                \
        if ((return_false))                                                                                            \
        {                                                                                                              \
            return false;                                                                                              \
        }                                                                                                              \
    }

#define TEST_MESSAGE(test_name) std::cout << "Testing " << test_name << "..." << std::endl
#define SUCCESS_MESSAGE(test_name) std::cout << test_name << " - DONE" << std::endl

// 跨平台臨時檔路徑:Windows 沒有 /tmp —— native MSVC 程式的 "/tmp/x" 是「無磁碟機的
// rooted path」,會解析成「目前磁碟機:\tmp\x」,開檔一律失敗。改用系統臨時目錄:
// Windows 用 TEMP/TMP,其餘平台用 TMPDIR(沒有才退回 /tmp)。
// 回傳的 const char * 在整個執行期有效:每個檔名各留一份字串,而 std::deque 在兩端插入
// 不會搬動既有元素([deque.modifiers]「has no effect on the validity of references to
// elements」),所以 paths.back().c_str() 不會被之後的 push_back 弄成懸空。
// 用法與舊字面值相同:const char *p = TestTempFilePath("chia_asset_a.bin");
inline const char *TestTempFilePath(const char *fileName)
{
    static const std::string dir = [] {
#ifdef _WIN32
        const char *env = std::getenv("TEMP");
        if (env == nullptr || *env == '\0')
            env = std::getenv("TMP");
#else
        const char *env = std::getenv("TMPDIR");
        if (env == nullptr || *env == '\0')
            env = "/tmp";
#endif
        if (env == nullptr || *env == '\0')
            return std::string(".");
        std::string d(env);
        // Windows 的檔案 API 同時接受 '/' 與 '\';統一補 '/',兩個平台共用同一份路徑組法。
        if (d.back() != '/' && d.back() != '\\')
            d += '/';
        return d;
    }();
    static std::deque<std::string> paths; // 只在 Run() 的主執行緒使用,不需同步
    paths.push_back(dir + fileName);
    return paths.back().c_str();
}

class Test
{
  private:
    std::string name;

  public:
    Test(const std::string &name = "") : name(name)
    {
    }

    std::string GetName() const
    {
        return name;
    }

    // Module owns std::vector<Test*> and deletes through Test*, so this must be virtual.
    virtual ~Test() = default;

    virtual bool Run() = 0;
};

#endif
