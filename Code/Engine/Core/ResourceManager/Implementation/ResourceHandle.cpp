#include <Core/CorePCH.h>

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

ezUInt64 ezTypelessResourceHandle::GetResourceIDHash() const
{
  return IsValid() ? m_pResource->GetResourceIDHash() : 0;
}

ezStringView ezTypelessResourceHandle::GetResourceID() const
{
  if (IsValid())
  {
    return m_pResource->GetResourceID();
  }

  return {};
}

ezStringView ezTypelessResourceHandle::GetResourceIdOrDescription() const
{
  if (IsValid())
  {
    return m_pResource->GetResourceIdOrDescription();
  }

  return {};
}

const ezRTTI* ezTypelessResourceHandle::GetResourceType() const
{
  return IsValid() ? m_pResource->GetDynamicRTTI() : nullptr;
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

// static
void ezResourceHandleStreamOperations::WriteHandle(ezStreamWriter& Stream, const ezResource* pResource)
{
  if (pResource != nullptr)
  {
    Stream << pResource->GetDynamicRTTI()->GetTypeName();
    Stream << pResource->GetResourceID();
  }
  else
  {
    const char* szEmpty = "";
    Stream << szEmpty;
  }
}

// static
void ezResourceHandleStreamOperations::ReadHandle(ezStreamReader& Stream, ezTypelessResourceHandle& ResourceHandle)
{
  ezStringBuilder sTemp;

  Stream >> sTemp;
  if (sTemp.IsEmpty())
  {
    ResourceHandle.Invalidate();
    return;
  }

  const ezRTTI* pRtti = ezResourceManager::FindResourceForAssetType(sTemp);

  if (pRtti == nullptr)
  {
    pRtti = ezRTTI::FindTypeByName(sTemp);
  }

  if (pRtti == nullptr)
  {
    ezLog::Error("Unknown resource type '{0}'", sTemp);
    ResourceHandle.Invalidate();
  }

  // read unique ID for restoring the resource (from file)
  Stream >> sTemp;

  if (pRtti != nullptr)
  {
    ResourceHandle = ezResourceManager::LoadResourceByType(pRtti, sTemp);
  }
}
