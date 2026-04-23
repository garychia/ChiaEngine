#include "pch.hpp"
#include <cstdio>
#include <sys/stat.h>

#ifdef FILE_HANDLE
#undef FILE_HANDLE
#endif
#define FILE_HANDLE FILE*

#ifdef READ_MODE
#undef READ_MODE
#endif
#define READ_MODE 1

#ifdef WRITE_MODE
#undef WRITE_MODE
#endif
#define WRITE_MODE 2

#ifdef READ_WRITE_MODE
#undef READ_WRITE_MODE
#endif
#define READ_WRITE_MODE 3

#ifdef ACCESS_CREATE_OR_OVERRIDE
#undef ACCESS_CREATE_OR_OVERRIDE
#endif
#define ACCESS_CREATE_OR_OVERRIDE 4

#ifdef CREATE_IF_NOT_EXIST
#undef CREATE_IF_NOT_EXIST
#endif
#define CREATE_IF_NOT_EXIST 5

#ifdef OPEN_ONLY_ACCESS
#undef OPEN_ONLY_ACCESS
#endif
#define OPEN_ONLY_ACCESS 6

#ifdef OVERRIDE_IF_EXIST
#undef OVERRIDE_IF_EXIST
#endif
#define OVERRIDE_IF_EXIST 7

inline const char* GetOpenModeString(unsigned long openMode, unsigned long accessMode)
{
    if (openMode == READ_MODE)
    {
        if (accessMode == OPEN_ONLY_ACCESS) return "r";
        if (accessMode == OVERRIDE_IF_EXIST) return "r";
        if (accessMode == CREATE_IF_NOT_EXIST) return "a";
        if (accessMode == ACCESS_CREATE_OR_OVERRIDE) return "w";
    }
    else if (openMode == WRITE_MODE)
    {
        if (accessMode == OPEN_ONLY_ACCESS) return "w";
        if (accessMode == OVERRIDE_IF_EXIST) return "w";
        if (accessMode == CREATE_IF_NOT_EXIST) return "a";
        if (accessMode == ACCESS_CREATE_OR_OVERRIDE) return "w";
    }
    else if (openMode == READ_WRITE_MODE)
    {
        if (accessMode == OPEN_ONLY_ACCESS) return "r+";
        if (accessMode == OVERRIDE_IF_EXIST) return "r+";
        if (accessMode == CREATE_IF_NOT_EXIST) return "a+";
        if (accessMode == ACCESS_CREATE_OR_OVERRIDE) return "w+";
    }
    return "r";
}

inline bool FILE_EXIST(const char* path)
{
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

#define OPEN_HANDLE(handle, fileName, openMode, accessMode)                                                              \
    handle = fopen((const char*)(fileName), GetOpenModeString(openMode, accessMode))

#define VALID_HANDLE(handle) (handle != nullptr)

inline bool RESET_FILE_PTR(FILE* handle)
{
    return fseek(handle, 0, SEEK_SET) == 0;
}

inline FILE* SET_FILE_PTR_TO_END(FILE* handle)
{
    return fseek(handle, 0, SEEK_END) ? nullptr : handle;
}

inline FILE* SET_FILE_PTR(FILE* handle, long distance)
{
    return fseek(handle, distance, SEEK_CUR) ? nullptr : handle;
}

#define VALID_FILE_PTR(ptr) (ptr != nullptr)

inline size_t READ_FILE(FILE* handle, void* buffer, size_t bytes, size_t* pBytesRead)
{
    *pBytesRead = fread(buffer, 1, bytes, handle);
    return *pBytesRead;
}

#define CloseHandle(handle) fclose(handle)