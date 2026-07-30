#include "System/IO/IO.hpp"
#include <cstring>

bool FileIO::Open()
{
    if (opened)
        return true;
    Str<char> utf8Path = path.ToUTF8();
    const char* fileName = utf8Path.CStr();
    
    if (!FILE_EXIST(fileName))
    {
        return false;
    }
    OPEN_HANDLE(handle, fileName, static_cast<unsigned long>(option.openMode), static_cast<unsigned long>(option.accessMode));
    
    if (!VALID_HANDLE(handle))
        return false;
    opened = true;
    if (option.append && !VALID_FILE_PTR(SET_FILE_PTR_TO_END(handle)))
    {
        Close();
        return false;
    }
    return true;
}

bool FileIO::IsOpened() const
{
    return opened;
}

void FileIO::Close()
{
    CloseHandle(handle);
    opened = false;
}