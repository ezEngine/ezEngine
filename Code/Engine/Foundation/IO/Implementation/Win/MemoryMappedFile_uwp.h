#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/PathUtils.h>

/// UWP does not support memory mapped files.

struct ezMemoryMappedFileImpl
{
  ezMemoryMappedFile::Mode m_Mode = ezMemoryMappedFile::Mode::None;
  void* m_pMappedFilePtr = nullptr;
  ezUInt64 m_uiFileSize = 0;

  ~ezMemoryMappedFileImpl() {}
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
  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

ezResult ezMemoryMappedFile::OpenShared(const char* szSharedName, ezUInt64 uiSize, Mode mode)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

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
  return m_Impl->m_pMappedFilePtr;
}

void* ezMemoryMappedFile::GetWritePointer(ezUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/)
{
  EZ_ASSERT_DEBUG(m_Impl->m_Mode >= Mode::ReadWrite, "File must be opened with read/write access before accessing it for writing.");
  return m_Impl->m_pMappedFilePtr;
}

ezUInt64 ezMemoryMappedFile::GetFileSize() const
{
  return m_Impl->m_uiFileSize;
}
