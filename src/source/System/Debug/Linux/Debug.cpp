#include "System/Debug/Debug.hpp"

void Debug::Print(const String &msg)
{
    auto utf8 = msg.ToUTF8();
    std::cout << utf8.CStr();
}

void Debug::Print(const wchar_t *msg)
{
    std::wcout << msg;
}

void Debug::PrintLine(const String &msg)
{
    Print(msg);
    std::cout << std::endl;
}

void Debug::PrintLine(const wchar_t *msg)
{
    Print(msg);
    std::wcout << std::endl;
}