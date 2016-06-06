#pragma once

#include <Core/Basics.h>
#include <Core/ResourceManager/ResourceBase.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/TaskSystem.h>


enum class ezResourceManagerEventType
{
  ManagerShuttingDown,
  ResourceCategoryChanged,
};


struct ezResourceManagerEvent
{
  ezResourceManagerEventType m_EventType;
  const ezResourceCategory* m_pCategory;
};

/// \brief [internal] Worker thread/task for loading resources from disk.
class EZ_CORE_DLL ezResourceManagerWorker : public ezTask
{
private:
  friend class ezResourceManager;

  ezResourceManagerWorker() { };

  static void DoWork(bool bCalledExternally);

  virtual void Execute() override;
};

/// \brief [internal] Worker thread/task for loading into the GPU.
class EZ_CORE_DLL ezResourceManagerWorkerGPU : public ezTask
{
public:
  ezResourceLoadData m_LoaderData;
  ezResourceBase* m_pResourceToLoad;
  ezResourceTypeLoader* m_pLoader;

private:
  friend class ezResourceManager;
  ezResourceManagerWorkerGPU() { };

  virtual void Execute() override;
};

class EZ_CORE_DLL ezResourceManager
{
public:

  static ezEvent<const ezResourceEvent&> s_ResourceEvents;
  static ezEvent<const ezResourceManagerEvent&> s_ManagerEvents;

  static void BroadcastResourceEvent(const ezResourceEvent& e);

  /// \brief Registers which resource type to use to load an asset with the given type name
  static void RegisterResourceForAssetType(const char* szAssetTypeName, const ezRTTI* pResourceType);

  /// \brief Returns the resource type that was registered to handle the given asset type for loading. nullptr if no resource type was registered for this asset type.
  static const ezRTTI* FindResourceForAssetType(const char* szAssetTypeName);

  /// \brief Same as LoadResource(), but instead of a template argument, the resource type to use is given as ezRTTI info. Returns a typeless handle due to the missing template argument.
  static ezTypelessResourceHandle LoadResourceByType(const ezRTTI* pResourceType, const char* szResourceID);

  /// \brief Returns a handle to the requested resource. szResourceID must uniquely identify the resource, different spellings will result in different resources.
  ///
  /// After the call to this function the resource definitely exists in memory. Upon access through BeginAcquireResource the resource will be loaded. If it is not possible to load the resource
  /// it will change to a 'missing' state. If the code accessing the resource cannot handle that case, the application will 'terminate' (that means crash).
  template<typename ResourceType>
  static ezTypedResourceHandle<ResourceType> LoadResource(const char* szResourceID);

  template<typename ResourceType>
  static ezTypedResourceHandle<ResourceType> LoadResource(const char* szResourceID, ezResourcePriority Priority, ezTypedResourceHandle<ResourceType> hFallbackResource);

  template<typename ResourceType>
  static ezTypedResourceHandle<ResourceType> CreateResource(const char* szResourceID, const typename ResourceType::DescriptorType& descriptor, const char* szResourceDescription = nullptr);

  template<typename ResourceType>
  static ezTypedResourceHandle<ResourceType> GetExistingResource(const char* szResourceID);

  template<typename ResourceType>
  static ResourceType* BeginAcquireResource(const ezTypedResourceHandle<ResourceType>& hResource, ezResourceAcquireMode mode = ezResourceAcquireMode::AllowFallback, const ezTypedResourceHandle<ResourceType>& hFallbackResource = ezTypedResourceHandle<ResourceType>(), ezResourcePriority Priority = ezResourcePriority::Unchanged);

  template<typename ResourceType>
  static void EndAcquireResource(ResourceType* pResource);

  template<typename ResourceType>
  static void SetResourceTypeLoader(ezResourceTypeLoader* pCreator);

  static void SetDefaultResourceLoader(ezResourceTypeLoader* pDefaultLoader);

  static ezResourceTypeLoader* GetDefaultResourceLoader() { return m_pDefaultResourceLoader; }

  static void OnEngineShutdown();

  static void OnCoreShutdown();

  static void OnCoreStartup();

  template<typename ResourceType>
  static void PreloadResource(const ezTypedResourceHandle<ResourceType>& hResource, ezTime tShouldBeAvailableIn);

  static void PreloadTypelessResource(const ezTypelessResourceHandle& hResource, ezTime tShouldBeAvailableIn);

  static ezUInt32 FreeUnusedResources(bool bFreeAllUnused);

  template<typename ResourceType>
  static void ReloadResource(const ezTypedResourceHandle<ResourceType>& hResource);

  template<typename ResourceType>
  static void ReloadResourcesOfType();

  static void ReloadResourcesOfType(const ezRTTI* pType);

  static void ReloadAllResources();

  //static void CleanUpResources();

  static void PerFrameUpdate();
  

  /// \brief Goes through all existing resources and broadcasts the 'Exists' event.
  /// Used to announce all currently existing resources to interested event listeners.
  static void BroadcastExistsEvent();

  /// \brief Sets up a new or existing category of resources.
  ///
  /// Each resource can be assigned to one category. All resources with the same category share the same total memory limits.
  static void ConfigureResourceCategory(const char* szCategoryName, ezUInt64 uiMemoryLimitCPU, ezUInt64 uiMemoryLimitGPU);

  static const ezResourceCategory& GetResourceCategory(const char* szCategoryName);

  /// \brief Registers a 'named' resource. When a resource is looked up using \a szLookupName, the lookup will be redirected to \a szRedirectionResource.
  ///
  /// This can be used to register a resource under an easier to use name. For example one can register "MenuBackground" as the name for "{ E50DCC85-D375-4999-9CFE-42F1377FAC85 }".
  /// If the lookup name already exists, it will be overwritten.
  static void RegisterNamedResource(const char* szLookupName, const char* szRedirectionResource);

  /// \brief Removes a previously registered name from the redirection table.
  static void UnregisterNamedResource(const char* szLookupName);

  /// \brief Returns the resource manager mutex. Allows to lock the manager on a thread when multiple operations need to be done in sequence.
  static ezMutex& GetMutex() { return s_ResourceMutex; }

private:
  friend class ezResourceManagerWorker;
  friend class ezResourceManagerWorkerGPU;
  friend class ezResourceHandleReadContext;

  static void ReloadResource(ezResourceBase* pResource);

  static void PreloadResource(ezResourceBase* pResource, ezTime tShouldBeAvailableIn);

  template<typename ResourceType>
  static ResourceType* GetResource(const char* szResourceID, bool bIsReloadable);

  static ezResourceBase* GetResource(const ezRTTI* pRtti, const char* szResourceID, bool bIsReloadable);

  static void InternalPreloadResource(ezResourceBase* pResource, bool bHighestPriority);

  static void RunWorkerTask(ezResourceBase* pResource);

  static void UpdateLoadingDeadlines();

  static ezResourceTypeLoader* GetResourceTypeLoader(const ezRTTI* pRTTI);

  static ezHashTable<ezTempHashedString, ezResourceBase*> m_LoadedResources;
  static ezMap<ezString, ezResourceTypeLoader*> m_ResourceTypeLoader;

  static ezResourceLoaderFromFile m_FileResourceLoader;

  static ezResourceTypeLoader* m_pDefaultResourceLoader;

  struct LoadingInfo
  {
    ezTime m_DueDate;
    ezResourceBase* m_pResource;

    EZ_FORCE_INLINE bool operator==(const LoadingInfo& rhs) const
    {
      return m_pResource == rhs.m_pResource;
    }

    inline bool operator<(const LoadingInfo& rhs) const
    {
      if (m_DueDate < rhs.m_DueDate)
        return true;
      if (m_DueDate > rhs.m_DueDate)
        return false;

      return m_pResource < rhs.m_pResource;
    }
  };

  static ezDeque<LoadingInfo> m_RequireLoading;

  static bool m_bTaskRunning;
  static bool m_bStop;
  static ezResourceManagerWorker m_WorkerTask[2];
  static ezResourceManagerWorkerGPU m_WorkerGPU[16];
  static ezUInt8 m_iCurrentWorkerGPU;
  static ezUInt8 m_iCurrentWorker;
  static ezTime m_LastDeadLineUpdate;
  static ezTime m_LastFrameUpdate;
  static bool m_bBroadcastExistsEvent;
  static ezHashTable<ezUInt32, ezResourceCategory> m_ResourceCategories;
  static ezMutex s_ResourceMutex;
  static ezHashTable<ezTempHashedString, ezHashedString> s_NamedResources;
  static ezMap<ezString, const ezRTTI*> s_AssetToResourceType;
};

/// \brief Helper class to acquire and release a resource safely.
///
/// The constructor calls ezResourceManager::BeginAcquireResource, the destructor makes sure to call ezResourceManager::EndAcquireResource.
/// The instance of this class can be used like a pointer to the resource.
template<class RESOURCE_TYPE>
class ezResourceLock
{
public:
  ezResourceLock(const ezTypedResourceHandle<RESOURCE_TYPE>& hResource, ezResourceAcquireMode mode = ezResourceAcquireMode::AllowFallback, const ezTypedResourceHandle<RESOURCE_TYPE>& hFallbackResource = ezTypedResourceHandle<RESOURCE_TYPE>(), ezResourcePriority Priority = ezResourcePriority::Unchanged)
  {
    m_pResource = ezResourceManager::BeginAcquireResource(hResource, mode, hFallbackResource, Priority);
  }

  ~ezResourceLock()
  {
    if (m_pResource)
      ezResourceManager::EndAcquireResource(m_pResource);
  }

  RESOURCE_TYPE* operator->()
  {
    return m_pResource;
  }

  operator bool()
  {
    return m_pResource != nullptr;
  }

private:
  RESOURCE_TYPE* m_pResource;
};

#include <Core/ResourceManager/Implementation/ResourceManager_inl.h>


