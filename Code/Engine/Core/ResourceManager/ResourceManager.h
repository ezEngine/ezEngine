#pragma once

#include <Core/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/TaskSystem.h>

/// \todo Do not unload resources while they are acquired
/// \todo Fallback resource for one type
/// \todo Missing resource for one type
/// \todo Events: Resource loaded / unloaded etc.
/// \todo Prevent loading of resource that should get created

// Resource Flags:
// Category / Group (Texture Sets)
//  Max Loaded Quality (adjustable at runtime)

// Resource Loader
//   Requires No File Access -> on non-File Thread
//   reload resource (if necessary)
//   

/// \brief [internal] Worker thread/task for loading resources from disk.
class EZ_CORE_DLL ezResourceManagerWorker : public ezTask
{
private:
  friend class ezResourceManager;

  ezResourceManagerWorker() { };

  virtual void Execute() EZ_OVERRIDE;
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

  virtual void Execute() EZ_OVERRIDE;
};



class EZ_CORE_DLL ezResourceManager
{
public:
  ezResourceManager();
  ~ezResourceManager();

  template<typename ResourceType>
  static ezResourceHandle<ResourceType> GetResourceHandle(const char* szResourceID);

  template<typename ResourceType>
  static void CreateResource(const ezResourceHandle<ResourceType>& hResource, const typename ResourceType::DescriptorType& descriptor);

  template<typename ResourceType>
  static ResourceType* BeginAcquireResource(const ezResourceHandle<ResourceType>& hResource, ezResourceAcquireMode::Enum mode = ezResourceAcquireMode::AllowFallback, ezResourcePriority::Enum Priority = ezResourcePriority::Unchanged);

  template<typename ResourceType>
  static void EndAcquireResource(ResourceType* pResource);

  template<typename ResourceType>
  static void SetResourceTypeLoader(ezResourceTypeLoader* pCreator);

  static void ezResourceManager::SetDefaultResourceLoader(ezResourceTypeLoader* pDefaultLoader);

  static ezResourceTypeLoader* ezResourceManager::GetDefaultResourceLoader() { return m_pDefaultResourceLoader; }

  static void Shutdown();

  template<typename ResourceType>
  static void PreloadResource(const ezResourceHandle<ResourceType>& hResource, ezTime tShouldBeAvailableIn);

  static ezUInt32 FreeUnusedResources();

  /// \todo ReloadResources of type / one / all (if necessary)

  //static void CleanUpResources();
  
private:
  friend class ezResourceManagerWorker;
  friend class ezResourceManagerWorkerGPU;

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

    inline bool operator<(const LoadingInfo& rhs) const
    {
      if (m_DueDate > rhs.m_DueDate)
        return true;
      if (m_DueDate < rhs.m_DueDate)
        return false;

      return m_pResource > rhs.m_pResource;
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
};

#include <Core/ResourceManager/Implementation/ResourceManager_inl.h>


