#include "System/IO/IO.hpp"

bool FileIO::Open()
{
    if (opened)
        return true;
    const String::CharType *fileName = path.CStr();
    if (!FILE_EXIST(fileName))
        return false;
    OPEN_HANDLE(handle, fileName, option.openMode, option.accessMode);
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
