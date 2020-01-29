#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <Core/ResourceManager/Implementation/Declarations.h>

/// \brief If this is set to EZ_ON, stack traces are recorded for every resource handle.
///
/// This can be used to find the places that create resource handles but do not properly clean them up.
#define EZ_RESOURCEHANDLE_STACK_TRACES EZ_OFF

class ezResource;

template <typename T>
class ezResourceLock;

// These out-of-line helper functions allow to forward declare resource handles without knowledge about the resource class.
EZ_CORE_DLL void IncreaseResourceRefCount(ezResource* pResource, const void* pOwner);
EZ_CORE_DLL void DecreaseResourceRefCount(ezResource* pResource, const void* pOwner);

#if EZ_ENABLED(EZ_RESOURCEHANDLE_STACK_TRACES)
EZ_CORE_DLL void MigrateResourceRefCount(ezResource* pResource, const void* pOldOwner, const void* pNewOwner);
#else
EZ_ALWAYS_INLINE void MigrateResourceRefCount(ezResource* pResource, const void* pOldOwner, const void* pNewOwner)
{
}
#endif

/// \brief The typeless implementation of resource handles. A typed interface is provided by ezTypedResourceHandle.
class EZ_CORE_DLL ezTypelessResourceHandle
{
public:
  EZ_ALWAYS_INLINE ezTypelessResourceHandle() = default;

  /// \brief [internal] Increases the refcount of the given resource.
  ezTypelessResourceHandle(ezResource* pResource);

  /// \brief Increases the refcount of the given resource
  EZ_ALWAYS_INLINE ezTypelessResourceHandle(const ezTypelessResourceHandle& rhs)
  {
    m_pResource = rhs.m_pResource;

    if (m_pResource)
    {
      IncreaseResourceRefCount(m_pResource, this);
    }
  }

  /// \brief Move constructor, no refcount change is necessary.
  EZ_ALWAYS_INLINE ezTypelessResourceHandle(ezTypelessResourceHandle&& rhs)
  {
    m_pResource = rhs.m_pResource;
    rhs.m_pResource = nullptr;

    if (m_pResource)
    {
      MigrateResourceRefCount(m_pResource, &rhs, this);
    }
  }

  /// \brief Releases any referenced resource.
  EZ_ALWAYS_INLINE ~ezTypelessResourceHandle() { Invalidate(); }

  /// \brief Returns whether the handle stores a valid pointer to a resource.
  EZ_ALWAYS_INLINE bool IsValid() const { return m_pResource != nullptr; }

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
  EZ_ALWAYS_INLINE bool operator==(const ezTypelessResourceHandle& rhs) const { return m_pResource == rhs.m_pResource; }

  /// \brief Checks whether the two handles point to the same resource.
  EZ_ALWAYS_INLINE bool operator!=(const ezTypelessResourceHandle& rhs) const { return m_pResource != rhs.m_pResource; }

  /// \brief For storing handles as keys in maps
  EZ_ALWAYS_INLINE bool operator<(const ezTypelessResourceHandle& rhs) const { return m_pResource < rhs.m_pResource; }

  /// \brief Checks whether the handle points to the given resource.
  EZ_ALWAYS_INLINE bool operator==(const ezResource* rhs) const { return m_pResource == rhs; }

  /// \brief Checks whether the handle points to the given resource.
  EZ_ALWAYS_INLINE bool operator!=(const ezResource* rhs) const { return m_pResource != rhs; }

protected:
  ezResource* m_pResource = nullptr;

private:
  // you must go through the resource manager to get access to the resource pointer
  friend class ezResourceManager;
  friend class ezResourceHandleWriteContext;
  friend class ezResourceHandleReadContext;
};

/// \brief The ezTypedResourceHandle controls access to an ezResource.
///
/// All resources must be referenced using ezTypedResourceHandle instances (instantiated with the proper resource type as the template
/// argument). You must not store a direct pointer to a resource anywhere. Instead always store resource handles. To actually access a
/// resource, use ezResourceManager::BeginAcquireResource and ezResourceManager::EndAcquireResource after you have finished using it.
///
/// ezTypedResourceHandle implements reference counting on resources. It also allows to redirect resources to fallback resources when they
/// are not yet loaded (if possible).
///
/// As long as there is one resource handle that references a resource, it is considered 'in use' and thus might not get unloaded.
/// So be careful where you store resource handles.
/// If necessary you can call Invalidate() to clear a resource handle and thus also remove the reference to the resource.
template <typename RESOURCE_TYPE>
class ezTypedResourceHandle
{
public:
  typedef RESOURCE_TYPE ResourceType;

  /// \brief A default constructed handle is invalid and does not reference any resource.
  ezTypedResourceHandle() {}

  /// \brief Increases the refcount of the given resource.
  explicit ezTypedResourceHandle(ResourceType* pResource)
    : m_Typeless(pResource)
  {
  }

  /// \brief Increases the refcount of the given resource.
  ezTypedResourceHandle(const ezTypedResourceHandle<ResourceType>& rhs)
    : m_Typeless(rhs.m_Typeless)
  {
  }

  /// \brief Move constructor, no refcount change is necessary.
  ezTypedResourceHandle(ezTypedResourceHandle<ResourceType>&& rhs)
    : m_Typeless(std::move(rhs.m_Typeless))
  {
  }

  template <typename BaseOrDerivedType>
  ezTypedResourceHandle(const ezTypedResourceHandle<BaseOrDerivedType>& rhs)
    : m_Typeless(rhs.m_Typeless)
  {
    static_assert(std::is_base_of<ResourceType, BaseOrDerivedType>::value || std::is_base_of<BaseOrDerivedType, ResourceType>::value,
      "Only related types can be assigned to handles of this type");

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (std::is_base_of<BaseOrDerivedType, ResourceType>::value)
    {
      EZ_ASSERT_DEBUG(rhs.IsValid(), "Cannot cast invalid base handle to derived type!");
      ezResourceLock<BaseOrDerivedType> lock(rhs, ezResourceAcquireMode::PointerOnly);
      EZ_ASSERT_DEBUG(ezDynamicCast<const ResourceType*>(lock.GetPointer()) != nullptr, "Types are not related!");
    }
#endif
  }

  /// \brief Releases the current reference and increases the refcount of the given resource.
  void operator=(const ezTypedResourceHandle<ResourceType>& rhs) { m_Typeless = rhs.m_Typeless; }

  /// \brief Move operator, no refcount change is necessary.
  void operator=(ezTypedResourceHandle<ResourceType>&& rhs) { m_Typeless = std::move(rhs.m_Typeless); }

  /// \brief Checks whether the two handles point to the same resource.
  EZ_ALWAYS_INLINE bool operator==(const ezTypedResourceHandle<ResourceType>& rhs) const { return m_Typeless == rhs.m_Typeless; }

  /// \brief Checks whether the two handles point to the same resource.
  EZ_ALWAYS_INLINE bool operator!=(const ezTypedResourceHandle<ResourceType>& rhs) const { return m_Typeless != rhs.m_Typeless; }

  /// \brief For storing handles as keys in maps
  EZ_ALWAYS_INLINE bool operator<(const ezTypedResourceHandle<ResourceType>& rhs) const { return m_Typeless < rhs.m_Typeless; }

  /// \brief Checks whether the handle points to the given resource.
  EZ_ALWAYS_INLINE bool operator==(const ezResource* rhs) const { return m_Typeless == rhs; }

  /// \brief Checks whether the handle points to the given resource.
  EZ_ALWAYS_INLINE bool operator!=(const ezResource* rhs) const { return m_Typeless != rhs; }


  /// \brief Returns the corresponding typeless resource handle.
  EZ_ALWAYS_INLINE operator const ezTypelessResourceHandle() const { return m_Typeless; }

  /// \brief Returns the corresponding typeless resource handle.
  EZ_ALWAYS_INLINE operator ezTypelessResourceHandle() { return m_Typeless; }

  /// \brief Returns whether the handle stores a valid pointer to a resource.
  EZ_ALWAYS_INLINE bool IsValid() const { return m_Typeless.IsValid(); }

  /// \brief Returns whether the handle stores a valid pointer to a resource.
  EZ_ALWAYS_INLINE explicit operator bool() const { return m_Typeless.IsValid(); }

  /// \brief Clears any reference to a resource and reduces its refcount.
  EZ_ALWAYS_INLINE void Invalidate() { m_Typeless.Invalidate(); }

  /// \brief Returns the Resource ID hash of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  EZ_ALWAYS_INLINE ezUInt32 GetResourceIDHash() const { return m_Typeless.GetResourceIDHash(); }

  /// \brief Returns the Resource ID of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  EZ_ALWAYS_INLINE const ezString& GetResourceID() const { return m_Typeless.GetResourceID(); }

private:
  template <typename T>
  friend class ezTypedResourceHandle;

  // you must go through the resource manager to get access to the resource pointer
  friend class ezResourceManager;
  friend class ezResourceHandleWriteContext;
  friend class ezResourceHandleReadContext;

  ezTypelessResourceHandle m_Typeless;
};

template <typename T>
struct ezHashHelper<ezTypedResourceHandle<T>>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const ezTypedResourceHandle<T>& value)
  {
    return value.GetResourceIDHash();
  }

  EZ_ALWAYS_INLINE static bool Equal(const ezTypedResourceHandle<T>& a, const ezTypedResourceHandle<T>& b) { return a == b; }
};
