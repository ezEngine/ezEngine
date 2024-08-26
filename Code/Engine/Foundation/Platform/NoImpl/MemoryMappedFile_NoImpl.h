#include <Foundation/FoundationPCH.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/PathUtils.h>

struct ezMemoryMappedFileImpl
{
  ezMemoryMappedFile::Mode m_Mode = ezMemoryMappedFile::Mode::None;
  void* m_pMappedFilePtr = nullptr;
  ezUInt64 m_uiFileSize = 0;

  ~ezMemoryMappedFileImpl() {}
};

ezMemoryMappedFile::ezMemoryMappedFile()
{
  m_pImpl = EZ_DEFAULT_NEW(ezMemoryMappedFileImpl);
}

ezMemoryMappedFile::~ezMemoryMappedFile()
{
  Close();
}

void ezMemoryMappedFile::Close()
{
  m_pImpl = EZ_DEFAULT_NEW(ezMemoryMappedFileImpl);
}

ezMemoryMappedFile::Mode ezMemoryMappedFile::GetMode() const
{
  return m_pImpl->m_Mode;
}

const void* ezMemoryMappedFile::GetReadPointer(ezUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/) const
{
  EZ_IGNORE_UNUSED(uiOffset);
  EZ_IGNORE_UNUSED(base);
  EZ_ASSERT_DEBUG(m_pImpl->m_Mode >= Mode::ReadOnly, "File must be opened with read access before accessing it for reading.");
  return m_pImpl->m_pMappedFilePtr;
}

void* ezMemoryMappedFile::GetWritePointer(ezUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/)
{
  EZ_IGNORE_UNUSED(uiOffset);
  EZ_IGNORE_UNUSED(base);
  EZ_ASSERT_DEBUG(m_pImpl->m_Mode >= Mode::ReadWrite, "File must be opened with read/write access before accessing it for writing.");
  return m_pImpl->m_pMappedFilePtr;
}

ezUInt64 ezMemoryMappedFile::GetFileSize() const
{
  return m_pImpl->m_uiFileSize;
}
