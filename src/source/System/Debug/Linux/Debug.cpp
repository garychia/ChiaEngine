#include "System/Debug/Debug.hpp"

#include <cstdint>
#include <cstring>
#include <string>

void Debug::Print(const String &msg)
{
    auto utf8 = msg.ToUTF8();
    std::cout << utf8.CStr();
}

void Debug::Print(const wchar_t *msg)
{
    // 統一走 std::cout(UTF-8):cout/wcout 雙流寫同一 fd 會交錯,
    // 把後續輸出弄丟/弄亂(validation layer 啟用後尤為明顯)。
    const size_t len = wcslen(msg);
    std::string utf8;
    utf8.reserve(len * 3);
    for (size_t i = 0; i < len; i++)
    {
        uint32_t cp = static_cast<uint32_t>(msg[i]);
        // surrogate pair
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < len && msg[i + 1] >= 0xDC00 && msg[i + 1] <= 0xDFFF)
        {
            cp = 0x10000 + ((cp - 0xD800) << 10) + (static_cast<uint32_t>(msg[i + 1]) - 0xDC00);
            i++;
        }
        if (cp < 0x80)
            utf8 += static_cast<char>(cp);
        else if (cp < 0x800)
        {
            utf8 += static_cast<char>(0xC0 | (cp >> 6));
            utf8 += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else if (cp < 0x10000)
        {
            utf8 += static_cast<char>(0xE0 | (cp >> 12));
            utf8 += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            utf8 += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else
        {
            utf8 += static_cast<char>(0xF0 | (cp >> 18));
            utf8 += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            utf8 += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            utf8 += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }
    std::cout << utf8;
}

void Debug::PrintLine(const String &msg)
{
    Print(msg);
    std::cout << std::endl;
}

void Debug::PrintLine(const wchar_t *msg)
{
    Print(msg);
    std::cout << std::endl;
}
