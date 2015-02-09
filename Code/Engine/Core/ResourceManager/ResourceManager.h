#pragma once

#include <Core/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/TaskSystem.h>


/// \brief [internal] Worker thread/task for loading resources from disk.
class EZ_CORE_DLL ezResourceManagerWorker : public ezTask
{
private:
  friend class ezResourceManager;

  ezResourceManagerWorker() { };

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
  template<typename ResourceType>
  static ezResourceHandle<ResourceType> LoadResource(const char* szResourceID);

  template<typename ResourceType>
  static ezResourceHandle<ResourceType> LoadResource(const char* szResourceID, ezResourcePriority Priority, ezResourceHandle<ResourceType> hFallbackResource);

  template<typename ResourceType>
  static ezResourceHandle<ResourceType> CreateResource(const char* szResourceID, const typename ResourceType::DescriptorType& descriptor);

  template<typename ResourceType>
  static ezResourceHandle<ResourceType> GetCreatedResource(const char* szResourceID);

  template<typename ResourceType>
  static ResourceType* BeginAcquireResource(const ezResourceHandle<ResourceType>& hResource, ezResourceAcquireMode mode = ezResourceAcquireMode::AllowFallback, const ezResourceHandle<ResourceType>& hFallbackResource = ezResourceHandle<ResourceType>(), ezResourcePriority Priority = ezResourcePriority::Unchanged);

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
  static void PreloadResource(const ezResourceHandle<ResourceType>& hResource, ezTime tShouldBeAvailableIn);

  static ezUInt32 FreeUnusedResources(bool bFreeAllUnused);

  template<typename ResourceType>
  static void ReloadResource(const ezResourceHandle<ResourceType>& hResource);

  template<typename ResourceType>
  static void ReloadResourcesOfType();

  static void ReloadResourcesOfType(const ezRTTI* pType);

  static void ReloadAllResources();

  //static void CleanUpResources();

  static void PerFrameUpdate();
  
private:
  friend class ezResourceManagerWorker;
  friend class ezResourceManagerWorkerGPU;

  static void ReloadResource(ezResourceBase* pResource);

  static void PreloadResource(ezResourceBase* pResource, ezTime tShouldBeAvailableIn);

  template<typename ResourceType>
  static ResourceType* GetResource(const char* szResourceID);

  static void InternalPreloadResource(ezResourceBase* pResource, bool bHighestPriority);

  static void RunWorkerTask();

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
  static ezInt8 m_iCurrentWorkerGPU;
  static ezInt8 m_iCurrentWorker;
  static ezTime m_LastDeadLineUpdate;
  static ezTime m_LastFrameUpdate;
};

/// \brief Helper class to acquire and release a resource safely.
///
/// The constructor calls ezResourceManager::BeginAcquireResource, the destructor makes sure to call ezResourceManager::EndAcquireResource.
/// The instance of this class can be used like a pointer to the resource.
template<class RESOURCE_TYPE>
class ezResourceLock
{
public:
  ezResourceLock(const ezResourceHandle<RESOURCE_TYPE>& hResource, ezResourceAcquireMode mode = ezResourceAcquireMode::AllowFallback, const ezResourceHandle<RESOURCE_TYPE>& hFallbackResource = ezResourceHandle<RESOURCE_TYPE>(), ezResourcePriority Priority = ezResourcePriority::Unchanged)
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

private:
  RESOURCE_TYPE* m_pResource;
};

#include <Core/ResourceManager/Implementation/ResourceManager_inl.h>


