#include "pch.hpp"

#define FILE_HANDLE HANDLE
#define READ_MODE GENERIC_READ
#define WRITE_MODE GENERIC_WRITE
#define READ_WRITE_MODE (GENERIC_READ | GENERIC_WRITE)

#define ACCESS_CREATE_OR_OVERRIDE CREATE_ALWAYS // Create a new file and override it if it exists.
#define CREATE_IF_NOT_EXIST CREATE_NEW          // Create only if the file does not exist.
#define OPEN_ONLY_ACCESS OPEN_EXISTING          // Open the file and fail if the file does not exist.
#define OVERRIDE_IF_EXIST TRUNCATE_EXISTING     // Override the file if it exists.

#define FILE_EXIST(path)                                                                                               \
    (!(INVALID_FILE_ATTRIBUTES == GetFileAttributes((LPCWSTR)(path)) && GetLastError() == ERROR_FILE_NOT_FOUND))
#define OPEN_HANDLE(handle, fileName, openMode, accessMode)                                                            \
    handle = CreateFile((LPCWSTR)(fileName), (DWORD)openMode, 0, NULL, (DWORD)accessMode, FILE_ATTRIBUTE_NORMAL, NULL)
#define VALID_HANDLE(handle) (handle != INVALID_HANDLE_VALUE)
#define RESET_FILE_PTR(handle) SetFilePointer(handle, 0, 0, FILE_BEGIN)
#define SET_FILE_PTR_TO_END(handle) SetFilePointer(handle, 0, 0, FILE_END)
#define SET_FILE_PTR(handle, distance) SetFilePointer(handle, (LONG)(distance), NULL, 1)
#define VALID_FILE_PTR(ptr) (!(ptr == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR))
#define READ_FILE(handle, buffer, bytes, pBytesRead) ReadFile(handle, buffer, bytes, pBytesRead, 0)
