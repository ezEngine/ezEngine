#include <Foundation/FoundationPCH.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/PathUtils.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <linux/version.h>
#endif

#if EZ_ENABLED(EZ_PLATFORM_OSX)
#  include <unistd.h>
#endif

struct ezMemoryMappedFileImpl
{
  ezMemoryMappedFile::Mode m_Mode = ezMemoryMappedFile::Mode::None;
  void* m_pMappedFilePtr = nullptr;
  ezUInt64 m_uiFileSize = 0;
  int m_hFile = -1;
  ezString m_sSharedMemoryName;

  ~ezMemoryMappedFileImpl()
  {
    if (m_pMappedFilePtr != nullptr)
    {
      munmap(m_pMappedFilePtr, m_uiFileSize);
      m_pMappedFilePtr = nullptr;
    }
    if (m_hFile != -1)
    {
      close(m_hFile);
      m_hFile = -1;
    }
#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
    // shm_open / shm_unlink deprecated.
    // There is an alternative in ASharedMemory_create but that is only
    // available in API 26 upwards.
#else
    if (!m_sSharedMemoryName.IsEmpty())
    {
      shm_unlink(m_sSharedMemoryName);
      m_sSharedMemoryName.Clear();
    }
#endif
    m_uiFileSize = 0;
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

#if EZ_ENABLED(EZ_SUPPORTS_MEMORY_MAPPED_FILE)
ezResult ezMemoryMappedFile::Open(const char* szAbsolutePath, Mode mode)
{
  EZ_ASSERT_DEV(mode != Mode::None, "Invalid mode to open the memory mapped file");
  EZ_ASSERT_DEV(ezPathUtils::IsAbsolutePath(szAbsolutePath), "ezMemoryMappedFile::Open() can only be used with absolute file paths");

  EZ_LOG_BLOCK("MemoryMapFile", szAbsolutePath);

  Close();

  m_Impl->m_Mode = mode;

  int access = O_RDONLY;
  int prot = PROT_READ;
  int flags = MAP_PRIVATE;
  if (mode == Mode::ReadWrite)
  {
    access = O_RDWR;
    prot |= PROT_WRITE;
    flags = MAP_SHARED;
  }
#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
#    if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 22)
  flags |= MAP_POPULATE;
#    endif
#  endif
  m_Impl->m_hFile = open(szAbsolutePath, access, 0);
  if (m_Impl->m_hFile == -1)
  {
    ezLog::Error("Could not open file for memory mapping - {}", strerror(errno));
    Close();
    return EZ_FAILURE;
  }
  struct stat sb;
  if (stat(szAbsolutePath, &sb) == -1 || sb.st_size == 0)
  {
    ezLog::Error("File for memory mapping is empty - {}", strerror(errno));
    Close();
    return EZ_FAILURE;
  }
  m_Impl->m_uiFileSize = sb.st_size;

  m_Impl->m_pMappedFilePtr = mmap(nullptr, m_Impl->m_uiFileSize,
    prot, flags, m_Impl->m_hFile, 0);
  if (m_Impl->m_pMappedFilePtr == nullptr)
  {
    ezLog::Error("Could not create memory mapping of file - {}", strerror(errno));
    Close();
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}
#endif

#if EZ_ENABLED(EZ_SUPPORTS_SHARED_MEMORY)
ezResult ezMemoryMappedFile::OpenShared(const char* szSharedName, ezUInt64 uiSize, Mode mode)
{
  EZ_ASSERT_DEV(mode != Mode::None, "Invalid mode to open the memory mapped file");
  EZ_ASSERT_DEV(uiSize > 0, "ezMemoryMappedFile::OpenShared() needs a valid file size to map");

  EZ_LOG_BLOCK("MemoryMapFile", szSharedName);

  Close();

  m_Impl->m_Mode = mode;

  int prot = PROT_READ;
  int oflag = O_RDONLY;
  int flags = MAP_SHARED;
#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
#    if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 22)
  flags |= MAP_POPULATE;
#    endif
#  endif

  if (mode == Mode::ReadWrite)
  {
    oflag = O_RDWR;
    prot |= PROT_WRITE;
  }
  oflag |= O_CREAT;

  m_Impl->m_hFile = shm_open(szSharedName, oflag, 0666);
  if (m_Impl->m_hFile == -1)
  {
    ezLog::Error("Could not open shared memory mapping - {}", strerror(errno));
    Close();
    return EZ_FAILURE;
  }
  m_Impl->m_sSharedMemoryName = szSharedName;

  if (ftruncate(m_Impl->m_hFile, uiSize) == -1)
  {
    ezLog::Error("Could not open shared memory mapping - {}", strerror(errno));
    Close();
    return EZ_FAILURE;
  }
  m_Impl->m_uiFileSize = uiSize;

  m_Impl->m_pMappedFilePtr = mmap(nullptr, m_Impl->m_uiFileSize,
    prot, flags, m_Impl->m_hFile, 0);
  if (m_Impl->m_pMappedFilePtr == nullptr)
  {
    ezLog::Error("Could not create memory mapping of file - {}", strerror(errno));
    Close();
    return EZ_FAILURE;
  }
  return EZ_SUCCESS;
}
#endif

void ezMemoryMappedFile::Close()
{
  m_Impl = EZ_DEFAULT_NEW(ezMemoryMappedFileImpl);
}

ezMemoryMappedFile::Mode ezMemoryMappedFile::GetMode() const
{
  return m_Impl->m_Mode;
}

const void* ezMemoryMappedFile::GetReadPointer(ezUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/) const
{
  EZ_ASSERT_DEBUG(m_Impl->m_Mode >= Mode::ReadOnly, "File must be opened with read access before accessing it for reading.");
  EZ_ASSERT_DEBUG(uiOffset <= m_Impl->m_uiFileSize, "Read offset must be smaller than mapped file size");

  if (base == OffsetBase::Start)
  {
    return ezMemoryUtils::AddByteOffset(m_Impl->m_pMappedFilePtr, uiOffset);
  }
  else
  {
    return ezMemoryUtils::AddByteOffset(m_Impl->m_pMappedFilePtr, m_Impl->m_uiFileSize - uiOffset);
  }
}

void* ezMemoryMappedFile::GetWritePointer(ezUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/)
{
  EZ_ASSERT_DEBUG(m_Impl->m_Mode >= Mode::ReadWrite, "File must be opened with read/write access before accessing it for writing.");
  EZ_ASSERT_DEBUG(uiOffset <= m_Impl->m_uiFileSize, "Read offset must be smaller than mapped file size");

  if (base == OffsetBase::Start)
  {
    return ezMemoryUtils::AddByteOffset(m_Impl->m_pMappedFilePtr, uiOffset);
  }
  else
  {
    return ezMemoryUtils::AddByteOffset(m_Impl->m_pMappedFilePtr, m_Impl->m_uiFileSize - uiOffset);
  }
}

ezUInt64 ezMemoryMappedFile::GetFileSize() const
{
  return m_Impl->m_uiFileSize;
}
