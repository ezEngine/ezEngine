#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>

/// \brief Helper class to acquire and release a resource safely.
///
/// The constructor calls ezResourceManager::BeginAcquireResource, the destructor makes sure to call ezResourceManager::EndAcquireResource.
/// The instance of this class can be used like a pointer to the resource.
template <class RESOURCE_TYPE>
class ezResourceLock
{
public:
  ezResourceLock(const ezTypedResourceHandle<RESOURCE_TYPE>& hResource, ezResourceAcquireMode mode = ezResourceAcquireMode::AllowFallback,
                 const ezTypedResourceHandle<RESOURCE_TYPE>& hFallbackResource = ezTypedResourceHandle<RESOURCE_TYPE>(),
                 ezResourcePriority Priority = ezResourcePriority::Unchanged)
  {
    m_pResource = ezResourceManager::BeginAcquireResource(hResource, mode, hFallbackResource, Priority, &m_AcquireResult);
  }

  ~ezResourceLock()
  {
    if (m_pResource)
      ezResourceManager::EndAcquireResource(m_pResource);
  }

  RESOURCE_TYPE* operator->() { return m_pResource; }

  operator bool() { return m_pResource != nullptr; }

  ezResourceAcquireResult GetAcquireResult() const { return m_AcquireResult; }

private:
  ezResourceAcquireResult m_AcquireResult;
  RESOURCE_TYPE* m_pResource;
};
