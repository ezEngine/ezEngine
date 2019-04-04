
#include <FoundationPCH.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/Memory/Allocator.h>

ezBlob::ezBlob() = default;

ezBlob::ezBlob(ezBlob&& other)
{
  m_pStorage = other.m_pStorage;
  m_uiSize = other.m_uiSize;

  other.m_pStorage = nullptr;
  other.m_uiSize = 0;
}

void ezBlob::operator=(ezBlob&& rhs)
{
  Clear();

  m_pStorage = rhs.m_pStorage;
  m_uiSize = rhs.m_uiSize;

  rhs.m_pStorage = nullptr;
  rhs.m_uiSize = 0;
}

ezBlob::~ezBlob()
{
  Clear();
}

void ezBlob::CopyFrom(void* pSource, ezUInt64 uiSize)
{
  SetCountUninitialized(uiSize);
  ezMemoryUtils::Copy(static_cast<ezUInt8*>(m_pStorage), static_cast<ezUInt8*>(pSource), uiSize);
}

void ezBlob::Clear()
{
  if(m_pStorage)
  {
    ezFoundation::GetAlignedAllocator()->Deallocate(m_pStorage);
    m_pStorage = nullptr;
    m_uiSize = 0;
  }
}

void ezBlob::SetCountUninitialized(ezUInt64 uiCount)
{
  if(m_uiSize != uiCount)
  {
    Clear();

    m_pStorage = ezFoundation::GetAlignedAllocator()->Allocate(uiCount, 64);
    m_uiSize = uiCount;
  }
}
