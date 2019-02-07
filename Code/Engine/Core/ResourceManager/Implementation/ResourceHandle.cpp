#include <CorePCH.h>

#include <Core/ResourceManager/Resource.h>

ezTypelessResourceHandle::ezTypelessResourceHandle(ezResource* pResource)
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

const ezString& ezTypelessResourceHandle::GetResourceID() const
{
  return m_pResource->GetResourceID();
}

void ezTypelessResourceHandle::operator=(const ezTypelessResourceHandle& rhs)
{
  EZ_ASSERT_DEBUG(this != &rhs, "Cannot assign a resource handle to itself! This would invalidate the handle.");

  Invalidate();

  m_pResource = rhs.m_pResource;

  if (m_pResource)
    IncreaseResourceRefCount(reinterpret_cast<ezResource*>(m_pResource));
}

void ezTypelessResourceHandle::operator=(ezTypelessResourceHandle&& rhs)
{
  Invalidate();

  m_pResource = rhs.m_pResource;
  rhs.m_pResource = nullptr;
}



EZ_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_ResourceHandle);

