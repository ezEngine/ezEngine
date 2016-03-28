#pragma once

#include <Core/Basics.h>

class ezResourceBase;

// These out-of-line helper functions allow to forward declare resource handles without knowledge about the resource class.
EZ_CORE_DLL void IncreaseResourceRefCount(ezResourceBase* pResource);
EZ_CORE_DLL void DecreaseResourceRefCount(ezResourceBase* pResource);

/// \brief The typeless implementation of resource handles. A typed interface is provided by ezTypedResourceHandle.
class EZ_CORE_DLL ezTypelessResourceHandle
{
public:
  EZ_FORCE_INLINE ezTypelessResourceHandle()
  {
    m_pResource = nullptr;
  }

  /// \brief [internal] Increases the refcount of the given resource.
  ezTypelessResourceHandle(ezResourceBase* pResource);

  /// \brief Increases the refcount of the given resource
  ezTypelessResourceHandle(const ezTypelessResourceHandle& rhs)
  {
    m_pResource = rhs.m_pResource;

    if (m_pResource)
      IncreaseResourceRefCount(reinterpret_cast<ezResourceBase*>(m_pResource));
  }

  /// \brief Move constructor, no refcount change is necessary.
  ezTypelessResourceHandle(ezTypelessResourceHandle&& rhs)
  {
    m_pResource = rhs.m_pResource;
    rhs.m_pResource = nullptr;
  }

  /// \brief Releases any referenced resource.
  EZ_FORCE_INLINE ~ezTypelessResourceHandle()
  {
    Invalidate();
  }

  /// \brief Returns whether the handle stores a valid pointer to a resource.
  EZ_FORCE_INLINE bool IsValid() const
  {
    return m_pResource != nullptr;
  }

  /// \brief Clears any reference to a resource and reduces its refcount.
  void Invalidate();

  /// \brief Returns the Resource ID hash of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  ezUInt32 GetResourceIDHash() const;

  /// \brief Returns the Resource ID of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  const ezString& GetResourceID() const;

  /// \brief Releases the current reference and increases the refcount of the given resource.
  void operator=(const ezTypelessResourceHandle& rhs);

  /// \brief Move operator, no refcount change is necessary.
  void operator=(ezTypelessResourceHandle&& rhs);

  /// \brief Checks whether the two handles point to the same resource.
  EZ_FORCE_INLINE bool operator==(const ezTypelessResourceHandle& rhs) const
  {
    return m_pResource == rhs.m_pResource;
  }

  /// \brief Checks whether the two handles point to the same resource.
  EZ_FORCE_INLINE bool operator!=(const ezTypelessResourceHandle& rhs) const
  {
    return m_pResource != rhs.m_pResource;
  }

protected:

  ezResourceBase* m_pResource;

private:
  // you must go through the resource manager to get access to the resource pointer
  friend class ezResourceManager;
  friend class ezResourceHandleWriteContext;
  friend class ezResourceHandleReadContext;
};

/// \brief The ezTypedResourceHandle controls access to an ezResource.
///
/// All resources must be referenced using ezTypedResourceHandle instances (instantiated with the proper resource type as the template argument).
/// You must not store a direct pointer to a resource anywhere. Instead always store resource handles. To actually access a resource,
/// use ezResourceManager::BeginAcquireResource and ezResourceManager::EndAcquireResource after you have finished using it.
///
/// ezTypedResourceHandle implements reference counting on resources. It also allows to redirect resources to fallback resources when they
/// are not yet loaded (if possible).
///
/// As long as there is one resource handle that references a resource, it is considered 'in use' and thus might not get unloaded.
/// So be careful where you store resource handles.
/// If necessary you can call Invalidate() to clear a resource handle and thus also remove the reference to the resource.
template<typename ResourceType>
class ezTypedResourceHandle
{
public:

  /// \brief A default constructed handle is invalid and does not reference any resource.
  ezTypedResourceHandle() {}

  /// \brief Increases the refcount of the given resource.
  ezTypedResourceHandle(ResourceType* pResource) : m_Typeless(pResource)
  {
  }

  /// \brief Increases the refcount of the given resource.
  ezTypedResourceHandle(const ezTypedResourceHandle<ResourceType>& rhs) : m_Typeless(rhs.m_Typeless)
  {
  }

  /// \brief Move constructor, no refcount change is necessary.
  ezTypedResourceHandle(ezTypedResourceHandle<ResourceType>&& rhs) 
    : m_Typeless(std::move(rhs.m_Typeless))
  {
  }

  /// \brief Releases the current reference and increases the refcount of the given resource.
  void operator=(const ezTypedResourceHandle<ResourceType>& rhs)
  {
    m_Typeless = rhs.m_Typeless;
  }

  /// \brief Move operator, no refcount change is necessary.
  void operator=(ezTypedResourceHandle<ResourceType>&& rhs)
  {
    m_Typeless = std::move(rhs.m_Typeless);
  }

  /// \brief Checks whether the two handles point to the same resource.
  EZ_FORCE_INLINE bool operator==(const ezTypedResourceHandle<ResourceType>& rhs) const
  {
    return m_Typeless == rhs.m_Typeless;
  }

  /// \brief Checks whether the two handles point to the same resource.
  EZ_FORCE_INLINE bool operator!=(const ezTypedResourceHandle<ResourceType>& rhs) const
  {
    return m_Typeless != rhs.m_Typeless;
  }


  /// \brief Returns whether the handle stores a valid pointer to a resource.
  EZ_FORCE_INLINE bool IsValid() const
  {
    return m_Typeless.IsValid();
  }

  /// \brief Clears any reference to a resource and reduces its refcount.
  EZ_FORCE_INLINE void Invalidate()
  {
    m_Typeless.Invalidate();
  }

  /// \brief Returns the Resource ID hash of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  EZ_FORCE_INLINE ezUInt32 GetResourceIDHash() const
  {
    return m_Typeless.GetResourceIDHash();
  }

  /// \brief Returns the Resource ID of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  EZ_FORCE_INLINE const ezString& GetResourceID() const
  {
    return m_Typeless.GetResourceID();
  }

private:
  ezTypelessResourceHandle m_Typeless;

private:
  // you must go through the resource manager to get access to the resource pointer
  friend class ezResourceManager;
  friend class ezResourceHandleWriteContext;
  friend class ezResourceHandleReadContext;
};


