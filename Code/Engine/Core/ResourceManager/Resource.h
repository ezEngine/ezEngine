#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Timestamp.h>

/// \brief The base class for all resources.
class EZ_CORE_DLL ezResource : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezResource, ezReflectedClass);

protected:
  enum class DoUpdate
  {
    OnMainThread,
    OnAnyThread
  };

  enum class Unload
  {
    AllQualityLevels,
    OneQualityLevel
  };

  /// \brief Default constructor.
  ezResource(DoUpdate ResourceUpdateThread, ezUInt8 uiQualityLevelsLoadable);

  /// \brief virtual destructor.
  virtual ~ezResource();

public:
  struct MemoryUsage
  {
    MemoryUsage()
    {
      m_uiMemoryCPU = 0;
      m_uiMemoryGPU = 0;
    }

    ezUInt32 m_uiMemoryCPU;
    ezUInt32 m_uiMemoryGPU;
  };

  /// \brief Returns the unique ID that identifies this resource. On a file resource this might be a path. Can also be a GUID or any other
  /// scheme that uniquely identifies the resource.
  EZ_ALWAYS_INLINE const ezString& GetResourceID() const { return m_UniqueID; }

  /// \brief Returns the hash of the unique ID.
  EZ_ALWAYS_INLINE ezUInt32 GetResourceIDHash() const { return m_uiUniqueIDHash; }

  /// \brief The resource description allows to store an additional string that might be more descriptive during debugging, than the unique
  /// ID.
  void SetResourceDescription(const char* szDescription);

  /// \brief The resource description allows to store an additional string that might be more descriptive during debugging, than the unique
  /// ID.
  const ezString& GetResourceDescription() const { return m_sResourceDescription; }

  /// \brief Returns the current state in which this resource is in.
  EZ_ALWAYS_INLINE ezResourceState GetLoadingState() const { return m_LoadingState; }

  /// \brief Returns the current maximum quality level that the resource could have.
  ///
  /// This is used to scale the amount data used. Once a resource is in the 'Loaded' state, it can still have different
  /// quality levels. E.g. a texture can be fully used with n mipmap levels, but there might be more that could be loaded.
  /// On the other hand a resource could have a higher 'loaded quality level' then the 'max quality level', if the user
  /// just changed settings and reduced the maximum quality level that should be used. In this case the resource manager
  /// will instruct the resource to unload some of its data soon.
  ///
  /// The quality level is a purely logical concept that can be handled very different by different resource types.
  /// E.g. a texture resource could theoretically use one quality level per available mipmap level. However, since
  /// the resource should generally be able to load and unload each quality level separately, it might make more sense
  /// for a texture resource, to use one quality level for everything up to 64*64, and then one quality level for each
  /// mipmap above that, which would result in 5 quality levels for a 1024*1024 texture.
  ///
  /// Most resource will have zero or one quality levels (which is the same) as they are either loaded or not.
  EZ_ALWAYS_INLINE ezUInt8 GetNumQualityLevelsDiscardable() const { return m_uiQualityLevelsDiscardable; }

  /// \brief Returns how many quality levels the resource may additionally load.
  EZ_ALWAYS_INLINE ezUInt8 GetNumQualityLevelsLoadable() const { return m_uiQualityLevelsLoadable; }

  /// \brief Returns the priority that is used by the resource manager to determine which resource to load next.
  float GetLoadingPriority(ezTime tNow) const;

  /// \brief Returns the current resource priority.
  ezResourcePriority GetPriority() const { return m_Priority; }

  /// \brief Changes the current resource priority.
  void SetPriority(ezResourcePriority priority);

  /// \brief Returns the basic flags for the resource type. Mostly used the resource manager.
  EZ_ALWAYS_INLINE const ezBitflags<ezResourceFlags>& GetBaseResourceFlags() const { return m_Flags; }

  /// \brief Returns the information about the current memory usage of the resource.
  EZ_ALWAYS_INLINE const MemoryUsage& GetMemoryUsage() const { return m_MemoryUsage; }

  /// \brief Returns the time at which the resource was (tried to be) acquired last.
  /// If a resource is acquired using ezResourceAcquireMode::PointerOnly, this does not update the last acquired time, since the resource is
  /// not acquired for full use.
  EZ_ALWAYS_INLINE ezTime GetLastAcquireTime() const { return m_LastAcquire; }

  /// \brief Returns the reference count of this resource.
  EZ_ALWAYS_INLINE ezInt32 GetReferenceCount() const { return m_iReferenceCount; }

  /// \brief Returns the modification date of the file from which this resource was loaded.
  ///
  /// The date may be invalid, if it cannot be retrieved or the resource was created and not loaded.
  EZ_ALWAYS_INLINE const ezTimestamp& GetLoadedFileModificationTime() const { return m_LoadedFileModificationTime; }

  /// \brief Returns the current value of the resource change counter.
  /// Can be used to detect whether the resource has changed since using it last time.
  ///
  /// The resource change counter is increased by calling IncResourceChangeCounter() or
  /// whenever the resource content is updated.
  EZ_ALWAYS_INLINE ezUInt32 GetCurrentResourceChangeCounter() const { return m_uiResourceChangeCounter; }

  /// \brief Allows to manually increase the resource change counter to signal that dependent code might need to update.
  EZ_ALWAYS_INLINE void IncResourceChangeCounter() { ++m_uiResourceChangeCounter; }

  /// \brief If the resource has modifications from the original state, it should reset itself to that state now (or force a reload on
  /// itself).
  virtual void ResetResource() {}

  /// \brief Prints the stack-traces for all handles that currently reference this resource.
  ///
  /// Only implemented if EZ_RESOURCEHANDLE_STACK_TRACES is EZ_ON.
  /// Otherwise the function does nothing.
  void PrintHandleStackTraces();


  mutable ezEvent<const ezResourceEvent&, ezMutex> m_ResourceEvents;

private:
  friend class ezResourceManager;
  friend class ezResourceManagerWorkerDataLoad;
  friend class ezResourceManagerWorkerUpdateContent;

  /// \brief Called by ezResourceManager shortly after resource creation.
  void SetUniqueID(const char* szUniqueID, bool bIsReloadable);

  void CallUnloadData(Unload WhatToUnload);

  /// \brief Requests the resource to unload another quality level. If bFullUnload is true, the resource should unload all data, because it
  /// is going to be deleted afterwards.
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) = 0;

  void CallUpdateContent(ezStreamReader* Stream);

  /// \brief Called whenever more data for the resource is available. The resource must read the stream to update it's data.
  ///
  /// pStream may be nullptr in case the resource data could not be found.
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* pStream) = 0;

  /// \brief Returns the resource type loader that should be used for this type of resource, unless it has been overridden on the
  /// ezResourceManager.
  ///
  /// By default, this redirects to ezResourceManager::GetDefaultResourceLoader. So there is one global default loader, that can be set
  /// on the resource manager. Overriding this function will then allow to use a different resource loader on a specific type.
  /// Additionally, one can override the resource loader from the outside, by setting it via ezResourceManager::SetResourceTypeLoader.
  /// That last method always takes precedence and allows to modify the behavior without modifying the code for the resource.
  /// But in the default case, the resource defines which loader is used.
  virtual ezResourceTypeLoader* GetDefaultResourceTypeLoader() const;

private:
  volatile ezResourceState m_LoadingState = ezResourceState::Unloaded;

  ezUInt8 m_uiQualityLevelsDiscardable = 0;
  ezUInt8 m_uiQualityLevelsLoadable = 0;


protected:
  /// \brief Non-const version for resources that want to write this variable directly.
  MemoryUsage& ModifyMemoryUsage() { return m_MemoryUsage; }

  /// \brief Call this to specify whether a resource is reloadable.
  ///
  /// By default all created resources are flagged as not reloadable.
  /// All resources loaded from file are automatically flagged as reloadable.
  void SetIsReloadable(bool bIsReloadable) { m_Flags.AddOrRemove(ezResourceFlags::IsReloadable, bIsReloadable); }

  /// \brief Used internally by the code injection macros
  void SetHasLoadingFallback(bool bHasLoadingFallback) { m_Flags.AddOrRemove(ezResourceFlags::ResourceHasFallback, bHasLoadingFallback); }

private:
  template <typename ResourceType>
  friend class ezTypedResourceHandle;

  friend EZ_CORE_DLL void IncreaseResourceRefCount(ezResource* pResource, const void* pOwner);
  friend EZ_CORE_DLL void DecreaseResourceRefCount(ezResource* pResource, const void* pOwner);

#if EZ_ENABLED(EZ_RESOURCEHANDLE_STACK_TRACES)
  friend EZ_CORE_DLL void MigrateResourceRefCount(ezResource* pResource, const void* pOldOwner, const void* pNewOwner);

  struct HandleStackTrace
  {
    ezUInt32 m_uiNumPtrs = 0;
    void* m_Ptrs[64];
  };

  ezMutex m_HandleStackTraceMutex;
  ezHashTable<const void*, HandleStackTrace> m_HandleStackTraces;
#endif


  /// \brief This function must be overridden by all resource types.
  ///
  /// It has to compute the memory used by this resource.
  /// It is called by the resource manager whenever the resource's data has been loaded or unloaded.
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) = 0;

  virtual void ReportResourceIsMissing();

  virtual bool HasResourceTypeLoadingFallback() const = 0;

  /// \brief Called by ezResourceMananger::CreateResource
  void VerifyAfterCreateResource(const ezResourceLoadDesc& ld);

  ezUInt32 m_uiUniqueIDHash = 0;
  ezUInt32 m_uiResourceChangeCounter = 0;
  ezAtomicInteger32 m_iReferenceCount = 0;
  ezAtomicInteger32 m_iLockCount = 0;
  ezString m_UniqueID;
  ezString m_sResourceDescription;
  MemoryUsage m_MemoryUsage;
  ezBitflags<ezResourceFlags> m_Flags;

  ezTime m_LastAcquire;
  ezResourcePriority m_Priority = ezResourcePriority::Medium;
  ezTimestamp m_LoadedFileModificationTime;

private:
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  static const ezResource* GetCurrentlyUpdatingContent();
#endif
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// GLORIOUS MACROS FOR RESOURCE CLASS CODE GENERATION
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Core/ResourceManager/ResourceManager.h>

#define EZ_RESOURCE_DECLARE_COMMON_CODE(SELF)                                                                                                \
  friend class ::ezResourceManager;                                                                                                          \
                                                                                                                                             \
public:                                                                                                                                      \
  /*                                                                                                                                     \ \ \
  /// \brief Unfortunately this has to be called manually from within dynamic plugins during core engine shutdown.                       \ \ \
  ///                                                                                                                                    \ \ \
  /// Without this, the dynamic plugin might still be referenced by the core engine during later shutdown phases and will crash, because \ \ \
  /// memory and code is still referenced, that is already unloaded.                                                                     \ \ \
  */                                                                                                                                         \
  static void CleanupDynamicPluginReferences();                                                                                              \
                                                                                                                                             \
  /*                                                                                                                                     \ \ \
  /// \brief Returns a typed resource handle to this resource                                                                            \ \ \
  */                                                                                                                                         \
  ezTypedResourceHandle<SELF> GetResourceHandle() const;                                                                                     \
                                                                                                                                             \
  /*                                                                                                                                     \ \ \
  /// \brief Sets the fallback resource that can be used while this resource is not yet loaded.                                          \ \ \
  ///                                                                                                                                    \ \ \
  /// By default there is no fallback resource, so all resource will block the application when requested for the first time.            \ \ \
  */                                                                                                                                         \
  void SetLoadingFallbackResource(const ezTypedResourceHandle<SELF>& hResource);                                                             \
                                                                                                                                             \
private:                                                                                                                                     \
  /* These functions are needed to access the static members, such that they get DLL exported, otherwise you get unresolved symbols */       \
  static void SetResourceTypeLoadingFallback(const ezTypedResourceHandle<SELF>& hResource);                                                  \
  static void SetResourceTypeMissingFallback(const ezTypedResourceHandle<SELF>& hResource);                                                  \
  static const ezTypedResourceHandle<SELF>& GetResourceTypeLoadingFallback() { return s_TypeLoadingFallback; }                               \
  static const ezTypedResourceHandle<SELF>& GetResourceTypeMissingFallback() { return s_TypeMissingFallback; }                               \
  virtual bool HasResourceTypeLoadingFallback() const override { return s_TypeLoadingFallback.IsValid(); }                                   \
                                                                                                                                             \
  static ezTypedResourceHandle<SELF> s_TypeLoadingFallback;                                                                                  \
  static ezTypedResourceHandle<SELF> s_TypeMissingFallback;                                                                                  \
                                                                                                                                             \
  ezTypedResourceHandle<SELF> m_hLoadingFallback;



#define EZ_RESOURCE_IMPLEMENT_COMMON_CODE(SELF)                                             \
  ezTypedResourceHandle<SELF> SELF::s_TypeLoadingFallback;                                  \
  ezTypedResourceHandle<SELF> SELF::s_TypeMissingFallback;                                  \
                                                                                            \
  void SELF::CleanupDynamicPluginReferences()                                               \
  {                                                                                         \
    s_TypeLoadingFallback.Invalidate();                                                     \
    s_TypeMissingFallback.Invalidate();                                                     \
    ezResourceManager::ClearResourceCleanupCallback(&SELF::CleanupDynamicPluginReferences); \
  }                                                                                         \
                                                                                            \
  ezTypedResourceHandle<SELF> SELF::GetResourceHandle() const                               \
  {                                                                                         \
    ezTypedResourceHandle<SELF> handle((SELF*)this);                                        \
    return handle;                                                                          \
  }                                                                                         \
                                                                                            \
  void SELF::SetLoadingFallbackResource(const ezTypedResourceHandle<SELF>& hResource)       \
  {                                                                                         \
    m_hLoadingFallback = hResource;                                                         \
    SetHasLoadingFallback(m_hLoadingFallback.IsValid());                                    \
  }                                                                                         \
                                                                                            \
  void SELF::SetResourceTypeLoadingFallback(const ezTypedResourceHandle<SELF>& hResource)   \
  {                                                                                         \
    s_TypeLoadingFallback = hResource;                                                      \
    EZ_RESOURCE_VALIDATE_FALLBACK(SELF);                                                    \
    ezResourceManager::AddResourceCleanupCallback(&SELF::CleanupDynamicPluginReferences);   \
  }                                                                                         \
  void SELF::SetResourceTypeMissingFallback(const ezTypedResourceHandle<SELF>& hResource)   \
  {                                                                                         \
    s_TypeMissingFallback = hResource;                                                      \
    EZ_RESOURCE_VALIDATE_FALLBACK(SELF);                                                    \
    ezResourceManager::AddResourceCleanupCallback(&SELF::CleanupDynamicPluginReferences);   \
  }


#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
#  define EZ_RESOURCE_VALIDATE_FALLBACK(SELF)                                       \
    if (hResource.IsValid())                                                        \
    {                                                                               \
      ezResourceLock<SELF> lock(hResource, ezResourceAcquireMode::BlockTillLoaded); \
      /* if this fails, the 'fallback resource' is missing itself*/                 \
    }
#else
#  define EZ_RESOURCE_VALIDATE_FALLBACK(SELF)
#endif

#define EZ_RESOURCE_DECLARE_CREATEABLE(SELF, SELF_DESCRIPTOR)      \
protected:                                                         \
  ezResourceLoadDesc CreateResource(SELF_DESCRIPTOR&& descriptor); \
                                                                   \
private:

#define EZ_RESOURCE_IMPLEMENT_CREATEABLE(SELF, SELF_DESCRIPTOR) ezResourceLoadDesc SELF::CreateResource(SELF_DESCRIPTOR&& descriptor)
