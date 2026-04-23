#include "System/IO/IO.hpp"
#include <cstring>

bool FileIO::Open()
{
    if (opened)
        return true;
    const char16_t* src = path.CStr();
    size_t len = path.Length();
    char* fileName = new char[len + 1];
    for (size_t i = 0; i < len; i++)
        fileName[i] = static_cast<char>(src[i]);
    fileName[len] = '\0';
    
    if (!FILE_EXIST(fileName))
    {
        delete[] fileName;
        return false;
    }
    OPEN_HANDLE(handle, fileName, static_cast<unsigned long>(option.openMode), static_cast<unsigned long>(option.accessMode));
    delete[] fileName;
    
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