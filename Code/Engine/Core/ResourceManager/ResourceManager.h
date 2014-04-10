#pragma once

#include <Core/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Threading/TaskSystem.h>

struct ezResourceLoadData
{
  ezStreamReaderBase* m_pDataStream;
  void* m_pCustomLoaderData;
};

class ezResourceTypeLoader
{
public:
  ezResourceTypeLoader() { }
  virtual ~ezResourceTypeLoader () { }

  virtual ezResourceLoadData OpenDataStream(const ezResourceBase* pResource) = 0;
  virtual void CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData) = 0;
};

class EZ_CORE_DLL ezResourceManagerWorker : public ezTask
{
private:
  virtual void Execute() EZ_OVERRIDE;
};

class EZ_CORE_DLL ezResourceManagerWorkerGPU : public ezTask
{
public:
  ezResourceLoadData m_LoaderData;
  ezResourceBase* m_pResourceToLoad;
  ezResourceTypeLoader* m_pLoader;

private:
  virtual void Execute() EZ_OVERRIDE;
};


struct EZ_CORE_DLL ezResourceAcquireMode
{
  enum Enum
  {
    PointerOnly,
    Loaded,
    LoadedNoFallback
  };
};



class EZ_CORE_DLL ezResourceManager
{
public:
  ezResourceManager();
  ~ezResourceManager();

  template<typename ResourceType>
  static ezResourceHandle<ResourceType> GetResourceHandle(const ezResourceID& ResourceID);

  template<typename ResourceType>
  static void CreateResource(const ezResourceHandle<ResourceType>& hResource, const typename ResourceType::DescriptorType& descriptor);

  template<typename ResourceType>
  static ResourceType* BeginAcquireResource(const ezResourceHandle<ResourceType>& hResource, ezResourceAcquireMode::Enum mode = ezResourceAcquireMode::Loaded, ezResourcePriority::Enum Priority = ezResourcePriority::Unchanged);

  template<typename ResourceType>
  static void EndAcquireResource(ResourceType* pResource);

  template<typename ResourceType>
  static void SetResourceTypeLoader(ezResourceTypeLoader* pCreator);

  static void Shutdown();

  template<typename ResourceType>
  static void PreloadResource(const ezResourceHandle<ResourceType>& hResource, ezTime tShouldBeAvailableIn);

  // FreeUnused

  static void CleanUpResources();
  
private:
  friend class ezResourceManagerWorker;
  friend class ezResourceManagerWorkerGPU;

  static void PreloadResource(ezResourceBase* pResource, bool bHighestPriority);

  static void RunWorkerTask();

  static void UpdateLoadingDeadlines();

  static ezResourceTypeLoader* GetResourceTypeLoader(const ezRTTI* pRTTI);

  static ezHashTable<ezResourceID, ezResourceBase*> m_LoadedResources;
  static ezMap<ezString, ezResourceTypeLoader*> m_ResourceTypeLoader;

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


