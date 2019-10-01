#include <CorePCH.h>

#include <Core/ResourceManager/Resource.h>

ezTypelessResourceHandle::ezTypelessResourceHandle(ezResource* pResource)
{
  m_pResource = pResource;

  if (m_pResource)
  {
    IncreaseResourceRefCount(m_pResource, this);
  }
}

void ezTypelessResourceHandle::Invalidate()
{
  if (m_pResource)
  {
    DecreaseResourceRefCount(m_pResource, this);
  }

  m_pResource = nullptr;
}

ezUInt32 ezTypelessResourceHandle::GetResourceIDHash() const
{
  return IsValid() ? m_pResource->GetResourceIDHash() : 0;
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
  {
    IncreaseResourceRefCount(reinterpret_cast<ezResource*>(m_pResource), this);
  }
}

void ezTypelessResourceHandle::operator=(ezTypelessResourceHandle&& rhs)
{
  Invalidate();

  m_pResource = rhs.m_pResource;
  rhs.m_pResource = nullptr;

  if (m_pResource)
  {
    MigrateResourceRefCount(m_pResource, &rhs, this);
  }
}



EZ_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_ResourceHandle);
