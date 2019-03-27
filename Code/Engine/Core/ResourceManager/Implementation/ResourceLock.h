#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>

/// \brief Helper class to acquire and release a resource safely.
///
/// The constructor calls ezResourceManager::BeginAcquireResource, the destructor makes sure to call ezResourceManager::EndAcquireResource.
/// The instance of this class can be used like a pointer to the resource.
///
/// Whether the acquisition succeeded or returned a loading fallback, missing fallback or even no result, at all,
/// can be retrieved through GetAcquireResult().
/// \note If a resource is missing, but no missing fallback is specified for the resource type, the code will fail with an assertion,
/// unless you used ezResourceAcquireMode::NoFallbackAllowMissing. Only then will the error be silently ignored and the acquire result
/// will be ezResourceAcquireResult::None.
///
/// \sa ezResourceManager::BeginAcquireResource()
/// \sa ezResourceAcquireMode
/// \sa ezResourceAcquireResult
template <class RESOURCE_TYPE>
class ezResourceLock
{
public:
  EZ_ALWAYS_INLINE ezResourceLock(const ezTypedResourceHandle<RESOURCE_TYPE>& hResource, ezResourceAcquireMode mode,
                 const ezTypedResourceHandle<RESOURCE_TYPE>& hFallbackResource = ezTypedResourceHandle<RESOURCE_TYPE>())
  {
    m_pResource = ezResourceManager::BeginAcquireResource(hResource, mode, hFallbackResource, &m_AcquireResult);
  }

  EZ_ALWAYS_INLINE ~ezResourceLock()
  {
    if (m_pResource)
    {
      ezResourceManager::EndAcquireResource(m_pResource);
    }
  }

  EZ_ALWAYS_INLINE RESOURCE_TYPE* operator->() { return m_pResource; }
  EZ_ALWAYS_INLINE const RESOURCE_TYPE* operator->() const { return m_pResource; }

  EZ_ALWAYS_INLINE bool IsValid() const { return m_pResource != nullptr; }
  EZ_ALWAYS_INLINE explicit operator bool() const { return m_pResource != nullptr; }

  EZ_ALWAYS_INLINE ezResourceAcquireResult GetAcquireResult() const { return m_AcquireResult; }

  EZ_ALWAYS_INLINE const RESOURCE_TYPE* GetPointer() const { return m_pResource; }
  EZ_ALWAYS_INLINE RESOURCE_TYPE* GetPointerNonConst() const { return m_pResource; }

private:
  ezResourceAcquireResult m_AcquireResult;
  RESOURCE_TYPE* m_pResource;
};

