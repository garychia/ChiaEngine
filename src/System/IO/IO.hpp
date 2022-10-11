#ifndef IO_HPP
#define IO_HPP

#include "Types/Types.hpp"

#if defined(_WIN32)
#include "System/IO/Windows/IOHelpers.hpp"
#elif (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
#else
#error Target operating system is not supported.
#endif

#include "Data/String.hpp"

enum class OpenMode : unsigned long
{
    Read = READ_MODE,
    Write = WRITE_MODE,
    ReadWrite = READ_WRITE_MODE
};

enum class AccessMode : unsigned long
{
    OpenOnly = OPEN_ONLY_ACCESS,
    OverrideExisting = OVERRIDE_IF_EXIST,
    CreateNonExsting = CREATE_IF_NOT_EXIST,
    CreateOverride = ACCESS_CREATE_OR_OVERRIDE
};

struct FileOption
{
    OpenMode openMode;
    AccessMode accessMode;
    bool append;
    FileOption(OpenMode mode = OpenMode::Read, AccessMode accessMode = AccessMode::OpenOnly, bool append = false)
        : openMode(mode), accessMode(accessMode), append(append)
    {
    }
};

class FileIO
{
  private:
    bool opened = false;
    bool endReached = false;
    FILE_HANDLE handle;
    String path;
    FileOption option;

  public:
    template <class StringType>
    FileIO(StringType &&path, const FileOption &option = FileOption())
        : path(Types::Forward<decltype(path)>(path)), option(option), handle(nullptr)
    {
    }

    bool Open();

    bool IsOpened() const;

    void Close();

    template <class BufferElement> unsigned long ReadBytes(unsigned long nBytes, BufferElement *buffer);

    template <class Char> Str<Char> ReadLine();
};

template <class BufferElement> unsigned long FileIO::ReadBytes(unsigned long nBytes, BufferElement *buffer)
{
    unsigned long bytesRead;
    return READ_FILE(handle, buffer, nBytes, &bytesRead) ? bytesRead : 0;
}

template <class Char> Str<Char> FileIO::ReadLine()
{
    if (endReached || !opened)
        return Str<Char>();
    const unsigned int bufferSize = 1024;
    unsigned char buffer[bufferSize];
    bool newLineFound = false;
    StrStream<Char> ss;
    while (!newLineFound)
    {
        const auto nBytesRead = ReadBytes(bufferSize, buffer);
        if (!nBytesRead)
        {
            endReached = true;
            break;
        }
        const auto nChars = nBytesRead / sizeof(Char);
        const Char *bufferPtr = (Char *)buffer;
        for (unsigned long i = 0; i < nChars; i++)
        {
            if (bufferPtr[i] == '\n' || bufferPtr[i] == '\r' && i + 1 < nChars && bufferPtr[i + 1] == '\n')
            {
                newLineFound = true;
                if (bufferPtr[i] == '\r')
                    i++;
                SET_FILE_PTR(handle, -(nChars - 1 - i) * sizeof(Char));
                break;
            }
            ss << bufferPtr[i];
        }
    }
    return ss.ToString();
}

#endif
