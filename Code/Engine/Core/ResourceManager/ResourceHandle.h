#pragma once

#include <Core/Basics.h>

class ezResourceBase;

// These out-of-line helper functions allow to forward declare resource handles without knowledge about the resource class.
EZ_CORE_DLL void IncreaseResourceRefCount(ezResourceBase* pResource);
EZ_CORE_DLL void DecreaseResourceRefCount(ezResourceBase* pResource);

/// \brief The ezResourceHandle controls access to an ezResource.
///
/// All resources must be referenced using ezResourceHandle instances (instantiated with the proper resource type as the template argument).
/// You must not store a direct pointer to a resource anywhere. Instead always store resource handles. To actually access a resource,
/// use ezResourceManager::BeginAcquireResource and ezResourceManager::EndAcquireResource after you have finished using it.
///
/// ezResourceHandle implements reference counting on resources. It also allows to redirect resources to fallback resources when they
/// are not yet loaded (if possible).
///
/// As long as there is one resource handle that references a resource, it is considered 'in use' and thus might not get unloaded.
/// So be careful where you store resource handles.
/// If necessary you can call Invalidate() to clear a resource handle and thus also remove the reference to the resource.
template<typename ResourceType>
class ezResourceHandle
{
public:

  /// \brief A default constructed handle is invalid and does not reference any resource.
  ezResourceHandle()
  {
    m_pResource = NULL;
  }

  /// \brief Increases the refcount of the given resource.
  ezResourceHandle(ResourceType* pResource)
  {
    m_pResource = pResource;

    if (m_pResource)
      IncreaseResourceRefCount(reinterpret_cast<ezResourceBase*>(m_pResource));
  }

  /// \brief Increases the refcount of the given resource.
  ezResourceHandle(const ezResourceHandle<ResourceType>& rhs)
  {
    m_pResource = rhs.m_pResource;

    if (m_pResource)
      IncreaseResourceRefCount(reinterpret_cast<ezResourceBase*>(m_pResource));
  }

  /// \brief Move constructor, no refcount change is necessary.
  ezResourceHandle(ezResourceHandle<ResourceType>&& rhs)
  {
    m_pResource = rhs.m_pResource;
    rhs.m_pResource = NULL;
  }

  /// \brief Releases the current reference and increases the refcount of the given resource.
  void operator=(const ezResourceHandle<ResourceType>& rhs)
  {
    Invalidate();

    m_pResource = rhs.m_pResource;

    if (m_pResource)
      IncreaseResourceRefCount(reinterpret_cast<ezResourceBase*>(m_pResource));
  }

  /// \brief Move operator, no refcount change is necessary.
  void operator=(ezResourceHandle<ResourceType>&& rhs)
  {
    Invalidate();

    m_pResource = rhs.m_pResource;
    rhs.m_pResource = NULL;
  }

  /// \brief Releases any referenced resource.
  ~ezResourceHandle()
  {
    Invalidate();
  }

  /// \brief Returns whether the handle stores a valid pointer to a resource.
  bool IsValid() const
  {
    return m_pResource != NULL;
  }

  /// \brief Clears any reference to a resource and reduces its refcount.
  void Invalidate()
  {
    if (m_pResource)
      DecreaseResourceRefCount(reinterpret_cast<ezResourceBase*>(m_pResource));

    m_pResource = NULL;
  }

  /// \brief Checks whether the two handles point to the same resource.
  EZ_FORCE_INLINE bool operator==(const ezResourceHandle<ResourceType>& rhs) const
  {
    return m_pResource == rhs.m_pResource;
  }

  /// \brief Checks whether the two handles point to the same resource.
  EZ_FORCE_INLINE bool operator!=(const ezResourceHandle<ResourceType>& rhs) const
  {
    return m_pResource != rhs.m_pResource;
  }

private:
  // you must go through the resource manager to get access to the resource pointer
  friend class ezResourceManager;

  ResourceType* m_pResource;
};


