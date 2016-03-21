#include <Core/PCH.h>
#include <Core/ResourceManager/ResourceBase.h>

ezTypelessResourceHandle::ezTypelessResourceHandle(ezResourceBase* pResource)
{
  m_pResource = pResource;

  if (m_pResource)
    IncreaseResourceRefCount(m_pResource);
}

void ezTypelessResourceHandle::Invalidate()
{
  if (m_pResource)
    DecreaseResourceRefCount(m_pResource);

  m_pResource = nullptr;
}

ezUInt32 ezTypelessResourceHandle::GetResourceIDHash() const
{
  return m_pResource->GetResourceIDHash();
}

void ezTypelessResourceHandle::operator=(const ezTypelessResourceHandle& rhs)
{
  Invalidate();

  m_pResource = rhs.m_pResource;

  if (m_pResource)
    IncreaseResourceRefCount(reinterpret_cast<ezResourceBase*>(m_pResource));
}

void ezTypelessResourceHandle::operator=(ezTypelessResourceHandle&& rhs)
{
  Invalidate();

  m_pResource = rhs.m_pResource;
  rhs.m_pResource = nullptr;
}

