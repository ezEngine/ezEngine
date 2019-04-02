#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringConversion.h>

struct ezMemoryMappedFileImpl
{
  ezMemoryMappedFile::Mode m_Mode = ezMemoryMappedFile::Mode::None;
  void* m_pMappedFilePtr = nullptr;
  ezUInt64 m_uiFileSize = 0;
  HANDLE m_hFile = INVALID_HANDLE_VALUE;
  HANDLE m_hMapping = INVALID_HANDLE_VALUE;

  ~ezMemoryMappedFileImpl()
  {
    if (m_pMappedFilePtr != nullptr)
    {
      UnmapViewOfFile(m_pMappedFilePtr);
      m_pMappedFilePtr = nullptr;
    }

    if (m_hMapping != INVALID_HANDLE_VALUE)
    {
      CloseHandle(m_hMapping);
      m_hMapping = INVALID_HANDLE_VALUE;
    }

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
      CloseHandle(m_hFile);
      m_hFile = INVALID_HANDLE_VALUE;
    }
  }
};

ezMemoryMappedFile::ezMemoryMappedFile()
{
  m_Impl = EZ_DEFAULT_NEW(ezMemoryMappedFileImpl);
}

ezMemoryMappedFile::~ezMemoryMappedFile()
{
  Close();
}

ezResult ezMemoryMappedFile::Open(const char* szAbsolutePath, Mode mode)
{
  EZ_ASSERT_DEV(mode != Mode::None, "Invalid mode to open the memory mapped file");
  EZ_ASSERT_DEV(ezPathUtils::IsAbsolutePath(szAbsolutePath), "ezMemoryMappedFile::Open() can only be used with absolute file paths");

  EZ_LOG_BLOCK("MemoryMapFile", szAbsolutePath);


  Close();

  m_Impl->m_Mode = mode;

  DWORD access = GENERIC_READ;

  if (mode == Mode::ReadWrite)
  {
    access |= GENERIC_WRITE;
  }

  m_Impl->m_hFile =
    CreateFileW(ezStringWChar(szAbsolutePath).GetData(), access, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

  DWORD errorCode = GetLastError();

  if (m_Impl->m_hFile == nullptr || m_Impl->m_hFile == INVALID_HANDLE_VALUE)
  {
    ezLog::Error("Could not open file for memory mapping - {}", ezArgErrorCode(errorCode));
    Close();
    return EZ_FAILURE;
  }

  if (GetFileSizeEx(m_Impl->m_hFile, reinterpret_cast<LARGE_INTEGER*>(&m_Impl->m_uiFileSize)) == FALSE || m_Impl->m_uiFileSize == 0)
  {
    ezLog::Error("File for memory mapping is empty");
    Close();
    return EZ_FAILURE;
  }

  m_Impl->m_hMapping =
    CreateFileMappingW(m_Impl->m_hFile, nullptr, m_Impl->m_Mode == Mode::ReadOnly ? PAGE_READONLY : PAGE_READWRITE, 0, 0, nullptr);

  if (m_Impl->m_hMapping == nullptr || m_Impl->m_hMapping == INVALID_HANDLE_VALUE)
  {
    errorCode = GetLastError();

    ezLog::Error("Could not create memory mapping of file - {}", ezArgErrorCode(errorCode));
    Close();
    return EZ_FAILURE;
  }

  m_Impl->m_pMappedFilePtr = MapViewOfFile(m_Impl->m_hMapping, mode == Mode::ReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE, 0, 0, 0);

  if (m_Impl->m_pMappedFilePtr == nullptr)
  {
    errorCode = GetLastError();

    ezLog::Error("Could not create memory mapping view of file - {}", ezArgErrorCode(errorCode));
    Close();
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezMemoryMappedFile::Close()
{
  m_Impl = EZ_DEFAULT_NEW(ezMemoryMappedFileImpl);
}

ezMemoryMappedFile::Mode ezMemoryMappedFile::GetMode() const
{
  return m_Impl->m_Mode;
}

const void* ezMemoryMappedFile::GetReadPointer() const
{
  EZ_ASSERT_DEBUG(m_Impl->m_Mode >= Mode::ReadOnly, "File must be opened with read access before accessing it for reading.");
  return m_Impl->m_pMappedFilePtr;
}

void* ezMemoryMappedFile::GetWritePointer()
{
  EZ_ASSERT_DEBUG(m_Impl->m_Mode >= Mode::ReadWrite, "File must be opened with read/write access before accessing it for writing.");
  return m_Impl->m_pMappedFilePtr;
}

ezUInt64 ezMemoryMappedFile::GetFileSize() const
{
  return m_Impl->m_uiFileSize;
}
