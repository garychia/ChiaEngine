#include "System/Debug/Debug.hpp"

#include <debugapi.h>

void Debug::Print(const String &msg)
{
    OutputDebugString((LPCWSTR)msg.CStr());
}

void Debug::Print(const wchar_t *msg)
{
    OutputDebugString((LPCWSTR)msg);
}

void Debug::PrintLine(const String &msg)
{
    OutputDebugString((LPCWSTR)msg.CStr());
    OutputDebugString(L"\n");
}

void Debug::PrintLine(const wchar_t *msg)
{
    OutputDebugString((LPCWSTR)msg);
    OutputDebugString(L"\n");
}

