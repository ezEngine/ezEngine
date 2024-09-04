#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/Blob.h>

#include <Foundation/Memory/AllocatorWithPolicy.h>

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

void ezBlob::SetFrom(const void* pSource, ezUInt64 uiSize)
{
  SetCountUninitialized(uiSize);
  ezMemoryUtils::Copy(static_cast<ezUInt8*>(m_pStorage), static_cast<const ezUInt8*>(pSource), static_cast<size_t>(uiSize));
}

void ezBlob::Clear()
{
  if (m_pStorage)
  {
    ezFoundation::GetAlignedAllocator()->Deallocate(m_pStorage);
    m_pStorage = nullptr;
    m_uiSize = 0;
  }
}

void ezBlob::SetCountUninitialized(ezUInt64 uiCount)
{
  if (m_uiSize != uiCount)
  {
    Clear();

    m_pStorage = ezFoundation::GetAlignedAllocator()->Allocate(ezMath::SafeConvertToSizeT(uiCount), 64u);
    m_uiSize = uiCount;
  }
}

void ezBlob::ZeroFill()
{
  if (m_pStorage)
  {
    ezMemoryUtils::ZeroFill(static_cast<ezUInt8*>(m_pStorage), static_cast<size_t>(m_uiSize));
  }
}

bool ezBlob::IsEmpty() const
{
  return 0 == m_uiSize;
}
