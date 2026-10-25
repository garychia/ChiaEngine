#include "pch.hpp"

#include <string>

#define FILE_HANDLE HANDLE
#define READ_MODE GENERIC_READ
#define WRITE_MODE GENERIC_WRITE
#define READ_WRITE_MODE (GENERIC_READ | GENERIC_WRITE)

#define ACCESS_CREATE_OR_OVERRIDE CREATE_ALWAYS // Create a new file and override it if it exists.
#define CREATE_NON_EXISTING CREATE_NEW          // Create only if the file does not exist.
#define OPEN_ONLY_ACCESS OPEN_EXISTING          // Open the file and fail if the file does not exist.
#define OVERRIDE_IF_EXISTS TRUNCATE_EXISTING     // Override the file if it exists.

// #131:下面的 API 是寬字元版(GetFileAttributesW / CreateFileW 吃 LPCWSTR),但
// FileIO::Open() 是平台中立的共用程式碼,交過來的是 UTF-8 窄字串(IO.cpp 先做
// path.ToUTF8())。舊寫法 (LPCWSTR)(path) 等於把 UTF-8 位元組當成 UTF-16 讀 →
// 路徑全毀,Windows 上 FileIO 從來開不了檔。
// (String 本身是 UTF-16,所以其他直接傳 String::CStr() 的地方是 16-bit 對 16-bit,
//  不受影響;這支是唯一真的壞掉的一條。)
// 解法:在後端把 UTF-8 轉成 UTF-16,IO.cpp 不必長出平台分支。
// IOToNativePath() 回傳的暫時 wstring 活到整個運算式結束,所以 .c_str() 在
// GetFileAttributesW / CreateFileW 呼叫期間都有效。
inline std::wstring IOToNativePath(const char *utf8)
{
    if (utf8 == nullptr || *utf8 == '\0')
        return std::wstring();
    // 先問長度(含結尾 0),再只轉不含結尾 0 的部分;不要寫入 std::wstring 的
    // data()+size(),那個位置標準禁止更動。
    const int sizeWithNull = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (sizeWithNull <= 1)
        return std::wstring();
    const int length = sizeWithNull - 1; // UTF-8 → UTF-16 的輸出不會比輸入位元組多
    std::wstring wide(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8, length, &wide[0], length);
    return wide;
}

#define FILE_EXIST(path)                                                                                               \
    (!(INVALID_FILE_ATTRIBUTES == GetFileAttributesW(IOToNativePath((path)).c_str()) &&                                \
       GetLastError() == ERROR_FILE_NOT_FOUND))
#define OPEN_HANDLE(handle, fileName, openMode, accessMode)                                                            \
    handle = CreateFileW(IOToNativePath((fileName)).c_str(), (DWORD)openMode, 0, NULL, (DWORD)accessMode,               \
                         FILE_ATTRIBUTE_NORMAL, NULL)
#define VALID_HANDLE(handle) (handle != INVALID_HANDLE_VALUE)
#define RESET_FILE_PTR(handle) SetFilePointer(handle, 0, 0, FILE_BEGIN)
#define SET_FILE_PTR_TO_END(handle) SetFilePointer(handle, 0, 0, FILE_END)
#define SET_FILE_PTR(handle, distance) SetFilePointer(handle, (LONG)(distance), NULL, 1)
#define VALID_FILE_PTR(ptr) (!(ptr == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR))
#define READ_FILE(handle, buffer, bytes, pBytesRead) ReadFile(handle, buffer, bytes, pBytesRead, 0)
