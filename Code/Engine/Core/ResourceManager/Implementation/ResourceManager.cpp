#include <PCH.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Communication/GlobalEvent.h>

/// \todo Do not unload resources while they are acquired
/// \todo Resource Type Memory Thresholds
/// \todo Preload does not load all quality levels
/// \todo Priority change on acquire not visible in inspector

/// Events:
///   Resource being loaded (Task) -> loading time

/// Infos to Display:
///   Ref Count (max)
///   Fallback: Type / Instance
///   Loading Time

/// Inspector -> Engine
///   Resource to Preview

// Resource Flags:
// Category / Group (Texture Sets)

// Resource Loader
//   Requires No File Access -> on non-File Thread



ezHashTable<ezTempHashedString, ezResourceBase*> ezResourceManager::m_LoadedResources;
ezMap<ezString, ezResourceTypeLoader*> ezResourceManager::m_ResourceTypeLoader;
ezResourceLoaderFromFile ezResourceManager::m_FileResourceLoader;
ezResourceTypeLoader* ezResourceManager::m_pDefaultResourceLoader = &m_FileResourceLoader;
ezDeque<ezResourceManager::LoadingInfo> ezResourceManager::m_RequireLoading;
bool ezResourceManager::m_bTaskRunning = false;
bool ezResourceManager::m_bStop = false;
ezResourceManagerWorkerDiskRead ezResourceManager::m_WorkerTasksDiskRead[MaxDiskReadTasks];
ezResourceManagerWorkerMainThread ezResourceManager::m_WorkerTasksMainThread[MaxMainThreadTasks];
ezUInt8 ezResourceManager::m_iCurrentWorkerMainThread = 0;
ezUInt8 ezResourceManager::m_iCurrentWorkerDiskRead = 0;
ezTime ezResourceManager::m_LastDeadLineUpdate;
ezTime ezResourceManager::m_LastFrameUpdate;
bool ezResourceManager::m_bBroadcastExistsEvent = false;
ezHashTable<ezUInt32, ezResourceCategory> ezResourceManager::m_ResourceCategories;
ezEvent<const ezResourceEvent&> ezResourceManager::s_ResourceEvents;
ezEvent<const ezResourceManagerEvent&> ezResourceManager::s_ManagerEvents;
ezMutex ezResourceManager::s_ResourceMutex;
ezHashTable<ezTempHashedString, ezHashedString> ezResourceManager::s_NamedResources;
ezMap<ezString, const ezRTTI*> ezResourceManager::s_AssetToResourceType;
ezMap<ezResourceBase*, ezUniquePtr<ezResourceTypeLoader> > ezResourceManager::s_CustomLoaders;
ezAtomicInteger32 ezResourceManager::s_ResourcesLoadedRecently;
ezAtomicInteger32 ezResourceManager::s_ResourcesInLoadingLimbo;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, ResourceManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezResourceManager::OnCoreStartup();
  }

  ON_CORE_SHUTDOWN
  {
    ezResourceManager::OnCoreShutdown();
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
    ezResourceManager::OnEngineShutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on


ezResourceTypeLoader* ezResourceManager::GetResourceTypeLoader(const ezRTTI* pRTTI)
{
  return m_ResourceTypeLoader[pRTTI->GetTypeName()];
}



void ezResourceManager::BroadcastResourceEvent(const ezResourceEvent& e)
{
  EZ_LOCK(s_ResourceMutex);

  // broadcast it through the resource to everyone directly interested in that specific resource
  e.m_pResource->m_ResourceEvents.Broadcast(e);

  // and then broadcast it to everyone else through the general event
  s_ResourceEvents.Broadcast(e);
}


void ezResourceManager::RegisterResourceForAssetType(const char* szAssetTypeName, const ezRTTI* pResourceType)
{
  ezStringBuilder s = szAssetTypeName;
  s.ToLower();

  s_AssetToResourceType[s] = pResourceType;
}

const ezRTTI* ezResourceManager::FindResourceForAssetType(const char* szAssetTypeName)
{
  ezStringBuilder s = szAssetTypeName;
  s.ToLower();

  return s_AssetToResourceType.GetValueOrDefault(s, nullptr);
}

void ezResourceManager::InternalPreloadResource(ezResourceBase* pResource, bool bHighestPriority)
{
  if (m_bStop)
    return;

  EZ_LOCK(s_ResourceMutex);

  // if there is nothing else that could be loaded, just return right away
  if (pResource->GetLoadingState() == ezResourceState::Loaded && pResource->GetNumQualityLevelsLoadable() == 0)
  {
    // due to the threading this can happen for all resource types and is valid
    //EZ_ASSERT_DEV(!pResource->m_Flags.IsSet(ezResourceFlags::IsPreloading), "Invalid flag on resource type '{0}'", pResource->GetDynamicRTTI()->GetTypeName());
    return;
  }

  // if we are already preloading this resource, but now it has highest priority
  // and it is still in the task queue (so not yet started)
  if (pResource->m_Flags.IsSet(ezResourceFlags::IsPreloading))
  {
    if (bHighestPriority)
    {
      LoadingInfo li;
      li.m_pResource = pResource;

      // move it to the front of the queue
      // if it is not in the queue anymore, it has already been started by some thread
      if (m_RequireLoading.Remove(li))
      {
        //ezLog::Info("Moving Resource to front of preloading queue");
        m_RequireLoading.PushFront(li);
      }
    }

    return;
  }

  //ezLog::Info("Resource not yet preloading, adding to queue");

  EZ_ASSERT_DEV(!pResource->m_Flags.IsSet(ezResourceFlags::IsPreloading), "");
  pResource->m_Flags.Add(ezResourceFlags::IsPreloading);

  LoadingInfo li;
  li.m_pResource = pResource;
  // not necessary here
  //li.m_DueDate = pResource->GetLoadingDeadline();

  if (bHighestPriority)
    m_RequireLoading.PushFront(li);
  else
    m_RequireLoading.PushBack(li);

  //ezLog::Debug("Adding resource '{0}' -> '{1}'to preload queue: {2} items", pResource->GetDynamicRTTI()->GetTypeName(), pResource->GetResourceID(), m_RequireLoading.GetCount());

  ezResourceEvent e;
  e.m_pResource = pResource;
  e.m_EventType = ezResourceEventType::ResourceInPreloadQueue;
  ezResourceManager::BroadcastResourceEvent(e);

  // the mutex will be released by RunWorkerTask
  RunWorkerTask(pResource);
}

void ezResourceManager::RunWorkerTask(ezResourceBase* pResource)
{
  if (m_bStop)
    return;

  bool bDoItYourself = false;

  // lock scope
  {
    EZ_LOCK(s_ResourceMutex);

    static bool bTaskNamesInitialized = false;

    if (!bTaskNamesInitialized)
    {
      bTaskNamesInitialized = true;
      ezStringBuilder s;

      for (ezUInt32 i = 0; i < MaxDiskReadTasks; ++i)
      {
        s.Format("Disk Resource Loader {0}", i);
        m_WorkerTasksDiskRead[i].SetTaskName(s.GetData());
      }

      for (ezUInt32 i = 0; i < MaxMainThreadTasks; ++i)
      {
        s.Format("Main Thread Resource Loader {0}", i);
        m_WorkerTasksMainThread[i].SetTaskName(s.GetData());
      }
    }

    if (pResource != nullptr && ezTaskSystem::IsLoadingThread())
    {
      bDoItYourself = true;
    }
    else
      if (!m_bTaskRunning && !ezResourceManager::m_RequireLoading.IsEmpty())
      {
        m_bTaskRunning = true;
        m_iCurrentWorkerDiskRead = (m_iCurrentWorkerDiskRead + 1) % MaxDiskReadTasks;
        ezTaskSystem::StartSingleTask(&m_WorkerTasksDiskRead[m_iCurrentWorkerDiskRead], ezTaskPriority::FileAccess);
      }
  }

  // this function is always called from within a mutex
  // but we need to release the mutex between every loop iteration to prevent deadlocks
  s_ResourceMutex.Release();

  while (bDoItYourself)
  {
    ezResourceManagerWorkerDiskRead::DoWork(true);

    {
      EZ_LOCK(s_ResourceMutex);

      if (pResource == nullptr || !pResource->m_Flags.IsAnySet(ezResourceFlags::IsPreloading))
      {
        //ezLog::Info("Resource not preloading anymore");
        break;
      }
    }
  }

  // reacquire to get into the proper state
  s_ResourceMutex.Acquire();
}

void ezResourceManager::UpdateLoadingDeadlines()
{
  // we are already in the s_ResourceMutex here

  /// \todo don't do this too often

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastDeadLineUpdate < ezTime::Milliseconds(100))
    return;

  m_LastDeadLineUpdate = tNow;

  if (m_RequireLoading.IsEmpty())
    return;

  ezLog::Debug("Updating Loading Deadlines");

  /// \todo Allow to tweak kick out time
  /// \todo Make sure resources that are queued here don't get deleted

  const ezTime tKickOut = tNow + ezTime::Seconds(30.0);

  ezUInt32 uiCount = m_RequireLoading.GetCount();
  for (ezUInt32 i = 0; i < uiCount; )
  {
    m_RequireLoading[i].m_DueDate = m_RequireLoading[i].m_pResource->GetLoadingDeadline(tNow);

    if (m_RequireLoading[i].m_DueDate > tKickOut)
    {
      EZ_ASSERT_DEV(m_RequireLoading[i].m_pResource->m_Flags.IsSet(ezResourceFlags::IsPreloading) == true, "");
      m_RequireLoading[i].m_pResource->m_Flags.Remove(ezResourceFlags::IsPreloading);

      ezResourceEvent e;
      e.m_pResource = m_RequireLoading[i].m_pResource;
      e.m_EventType = ezResourceEventType::ResourceOutOfPreloadQueue;
      ezResourceManager::BroadcastResourceEvent(e);

      //ezLog::Warning("Removing resource from preload queue due to time out");
      m_RequireLoading.RemoveAtSwap(i);
      --uiCount;
    }
    else
      ++i;
  }

  m_RequireLoading.Sort();
}

void ezResourceManagerWorkerMainThread::Execute()
{
  if (!m_LoaderData.m_sResourceDescription.IsEmpty())
    m_pResourceToLoad->SetResourceDescription(m_LoaderData.m_sResourceDescription);

  //ezLog::Debug("Updating Resource Content");
  ezResourceManager::s_ResourcesLoadedRecently.Increment();
  m_pResourceToLoad->CallUpdateContent(m_LoaderData.m_pDataStream);

  // update the file modification date, if available
  if (m_LoaderData.m_LoadedFileModificationDate.IsValid())
    m_pResourceToLoad->m_LoadedFileModificationTime = m_LoaderData.m_LoadedFileModificationDate;

  EZ_ASSERT_DEV(m_pResourceToLoad->GetLoadingState() != ezResourceState::Unloaded, "The resource should have changed its loading state.");

  // Update Memory Usage
  {
    ezResourceBase::MemoryUsage MemUsage;
    MemUsage.m_uiMemoryCPU = 0xFFFFFFFF;
    MemUsage.m_uiMemoryGPU = 0xFFFFFFFF;
    m_pResourceToLoad->UpdateMemoryUsage(MemUsage);

    EZ_ASSERT_DEV(MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its CPU memory usage", m_pResourceToLoad->GetResourceID());
    EZ_ASSERT_DEV(MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its GPU memory usage", m_pResourceToLoad->GetResourceID());

    m_pResourceToLoad->m_MemoryUsage = MemUsage;
  }

  m_pLoader->CloseDataStream(m_pResourceToLoad, m_LoaderData);

  {
    EZ_LOCK(ezResourceManager::s_ResourceMutex);
    EZ_ASSERT_DEV(m_pResourceToLoad->m_Flags.IsSet(ezResourceFlags::IsPreloading) == true, "");
    m_pResourceToLoad->m_Flags.Remove(ezResourceFlags::IsPreloading);

    //ezLog::Dev("Finished loading resource on main thread");
    EZ_ASSERT_DEV(ezResourceManager::s_ResourcesInLoadingLimbo > 0, "ezResourceManager::s_ResourcesInLoadingLimbo is incorrect");
    ezResourceManager::s_ResourcesInLoadingLimbo.Decrement();

    ezResourceEvent e;
    e.m_pResource = m_pResourceToLoad;
    e.m_EventType = ezResourceEventType::ResourceOutOfPreloadQueue;
    ezResourceManager::BroadcastResourceEvent(e);
  }

  m_pLoader = nullptr;
  m_pResourceToLoad = nullptr;
}

void ezResourceManagerWorkerDiskRead::Execute()
{
  DoWork(false);
}

void ezResourceManagerWorkerDiskRead::DoWork(bool bCalledExternally)
{
  ezResourceBase* pResourceToLoad = nullptr;
  ezResourceTypeLoader* pLoader = nullptr;
  ezUniquePtr<ezResourceTypeLoader> pCustomLoader;

  {
    EZ_LOCK(ezResourceManager::s_ResourceMutex);

    ezResourceManager::UpdateLoadingDeadlines();

    if (ezResourceManager::m_RequireLoading.IsEmpty())
    {
      ezResourceManager::m_bTaskRunning = false;
      return;
    }

    // set this before we remove the resource from the queue
    ezResourceManager::s_ResourcesInLoadingLimbo.Increment();

    auto it = ezResourceManager::m_RequireLoading.PeekFront();
    pResourceToLoad = it.m_pResource;
    ezResourceManager::m_RequireLoading.PopFront();

    //ezLog::Warning("Task taking item out of preload queue: {0} items remain", ezResourceManager::m_RequireLoading.GetCount());


    if (pResourceToLoad->m_Flags.IsSet(ezResourceFlags::HasCustomDataLoader))
    {
      pCustomLoader = std::move(ezResourceManager::s_CustomLoaders[pResourceToLoad]);
      pLoader = pCustomLoader.Borrow();
      pResourceToLoad->m_Flags.Remove(ezResourceFlags::HasCustomDataLoader);
      pResourceToLoad->m_Flags.Add(ezResourceFlags::PreventFileReload);
    }
  }

  if (pLoader == nullptr)
    pLoader = ezResourceManager::GetResourceTypeLoader(pResourceToLoad->GetDynamicRTTI());

  if (pLoader == nullptr)
    pLoader = pResourceToLoad->GetDefaultResourceTypeLoader();

  EZ_ASSERT_DEV(pLoader != nullptr, "No Loader function available for Resource Type '{0}'", pResourceToLoad->GetDynamicRTTI()->GetTypeName());

  ezResourceLoadData LoaderData = pLoader->OpenDataStream(pResourceToLoad);

  // the resource data has been loaded (at least one piece), reset the due date
  pResourceToLoad->SetDueDate();

  // we need this info later to do some work in a lock, all the directly following code is outside the lock
  const bool bResourceIsLoadedOnMainThread = pResourceToLoad->GetBaseResourceFlags().IsAnySet(ezResourceFlags::UpdateOnMainThread);

  if (bResourceIsLoadedOnMainThread)
  {
    ezResourceManagerWorkerMainThread* pWorkerMainThread = nullptr;

    {
      EZ_LOCK(ezResourceManager::s_ResourceMutex);

      pWorkerMainThread = &ezResourceManager::m_WorkerTasksMainThread[ezResourceManager::m_iCurrentWorkerMainThread];
      ezResourceManager::m_iCurrentWorkerMainThread = (ezResourceManager::m_iCurrentWorkerMainThread + 1) % ezResourceManager::MaxMainThreadTasks;
    }

    /// \todo This part is still not thread safe
    ezTaskSystem::WaitForTask(pWorkerMainThread);

    {
      EZ_LOCK(ezResourceManager::s_ResourceMutex);

      pWorkerMainThread->m_LoaderData = LoaderData;
      pWorkerMainThread->m_pLoader = pLoader;
      pWorkerMainThread->m_pCustomLoader = std::move(pCustomLoader);
      pWorkerMainThread->m_pResourceToLoad = pResourceToLoad;

      // the resource stays in 'limbo' until the main thread worker is finished with it
      ezTaskSystem::StartSingleTask(pWorkerMainThread, ezTaskPriority::SomeFrameMainThread);
    }
  }
  else
  {
    if (!LoaderData.m_sResourceDescription.IsEmpty())
      pResourceToLoad->SetResourceDescription(LoaderData.m_sResourceDescription);

    //ezLog::Debug("Updating Resource Content");
    ezResourceManager::s_ResourcesLoadedRecently.Increment();
    pResourceToLoad->CallUpdateContent(LoaderData.m_pDataStream);

    // update the file modification date, if available
    if (LoaderData.m_LoadedFileModificationDate.IsValid())
      pResourceToLoad->m_LoadedFileModificationTime = LoaderData.m_LoadedFileModificationDate;

    EZ_ASSERT_DEV(pResourceToLoad->GetLoadingState() != ezResourceState::Unloaded, "The resource should have changed its loading state.");

    // Update Memory Usage
    {
      ezResourceBase::MemoryUsage MemUsage;
      MemUsage.m_uiMemoryCPU = 0xFFFFFFFF;
      MemUsage.m_uiMemoryGPU = 0xFFFFFFFF;
      pResourceToLoad->UpdateMemoryUsage(MemUsage);

      EZ_ASSERT_DEV(MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its CPU memory usage", pResourceToLoad->GetResourceID());
      EZ_ASSERT_DEV(MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its GPU memory usage", pResourceToLoad->GetResourceID());

      pResourceToLoad->m_MemoryUsage = MemUsage;
    }

    pLoader->CloseDataStream(pResourceToLoad, LoaderData);
  }

  // all this will happen inside a lock
  {
    EZ_LOCK(ezResourceManager::s_ResourceMutex);

    if (!bResourceIsLoadedOnMainThread)
    {
      // this resource was finished loading, so we can immediately reduce the limbo counter
      ezResourceManager::s_ResourcesInLoadingLimbo.Decrement();

      //ezLog::Warning("Resource removed from preload queue");

      EZ_ASSERT_DEV(pResourceToLoad->m_Flags.IsSet(ezResourceFlags::IsPreloading) == true, "");
      pResourceToLoad->m_Flags.Remove(ezResourceFlags::IsPreloading);

      ezResourceEvent e;
      e.m_pResource = pResourceToLoad;
      e.m_EventType = ezResourceEventType::ResourceOutOfPreloadQueue;
      ezResourceManager::BroadcastResourceEvent(e);
    }

    if (!bCalledExternally)
    {
      ezResourceManager::m_bTaskRunning = false;
      ezResourceManager::RunWorkerTask(nullptr);
    }

    pCustomLoader.Reset();
  }
}

ezUInt32 ezResourceManager::FreeUnusedResources(bool bFreeAllUnused)
{
  EZ_LOCK(s_ResourceMutex);
  EZ_LOG_BLOCK("ezResourceManager::FreeUnusedResources");

  ezUInt32 uiUnloaded = 0;
  bool bUnloadedAny = false;

  do
  {
    bUnloadedAny = false;

    for (auto it = m_LoadedResources.GetIterator(); it.IsValid(); /* empty */)
    {
      ezResourceBase* pReference = it.Value();

      if (pReference->m_iReferenceCount > 0)
      {
        ++it;
        continue;
      }

      const auto& CurKey = it.Key();

      EZ_ASSERT_DEV(pReference->m_iLockCount == 0, "Resource '{0}' has a refcount of zero, but is still in an acquired state.", pReference->GetResourceID());

      bUnloadedAny = true;
      ++uiUnloaded;
      pReference->CallUnloadData(ezResourceBase::Unload::AllQualityLevels);

      EZ_ASSERT_DEV(pReference->GetLoadingState() <= ezResourceState::UnloadedMetaInfoAvailable, "Resource '{0}' should be in an unloaded state now.", pReference->GetResourceID());

      // broadcast that we are going to delete the resource
      {
        ezResourceEvent e;
        e.m_pResource = pReference;
        e.m_EventType = ezResourceEventType::ResourceDeleted;
        ezResourceManager::BroadcastResourceEvent(e);
      }

      // delete the resource via the RTTI provided allocator
      pReference->GetDynamicRTTI()->GetAllocator()->Deallocate(pReference);

      ++it;

      m_LoadedResources.Remove(CurKey);
    }
  }
  while (bFreeAllUnused && bUnloadedAny);

  return uiUnloaded;
}

void ezResourceManager::PreloadResource(ezResourceBase* pResource, ezTime tShouldBeAvailableIn)
{
  const ezTime tNow = m_LastFrameUpdate;

  pResource->SetDueDate(ezMath::Min(tNow + tShouldBeAvailableIn, pResource->m_DueDate));
  InternalPreloadResource(pResource, tShouldBeAvailableIn <= ezTime::Seconds(0.0)); // if the user set the timeout to zero or below, it will be scheduled immediately
}


void ezResourceManager::PreloadResource(const ezTypelessResourceHandle& hResource, ezTime tShouldBeAvailableIn)
{
  // this is the same as BeginAcquireResource in PointerOnly mode

  EZ_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

  ezResourceBase* pResource = hResource.m_pResource;
  EZ_ASSERT_DEV(pResource->m_iLockCount < 20, "You probably forgot somewhere to call 'EndAcquireResource' in sync with 'BeginAcquireResource'.");

  {
    pResource->m_iLockCount.Increment();
    PreloadResource(hResource.m_pResource, tShouldBeAvailableIn);
    pResource->m_iLockCount.Decrement();
  }
}

bool ezResourceManager::ReloadResource(ezResourceBase* pResource, bool bForce)
{
  EZ_LOCK(s_ResourceMutex);

  if (!pResource->m_Flags.IsAnySet(ezResourceFlags::IsReloadable))
    return false;

  if (!bForce && pResource->m_Flags.IsAnySet(ezResourceFlags::PreventFileReload))
      return false;

  ezResourceTypeLoader* pLoader = ezResourceManager::GetResourceTypeLoader(pResource->GetDynamicRTTI());

  /// \todo Do we need to handle HasCustomDataLoader here ?? (apparently not)

  if (pLoader == nullptr)
    pLoader = pResource->GetDefaultResourceTypeLoader();

  if (pLoader == nullptr)
    return false;

  // no need to reload resources that are not loaded so far
  if (pResource->GetLoadingState() == ezResourceState::Unloaded)
    return false;

  bool bAllowPreloading = true;

  // if the resource is already in the preloading queue we can just keep it there
  if (pResource->m_Flags.IsSet(ezResourceFlags::IsPreloading))
  {
    bAllowPreloading = false;

    LoadingInfo li;
    li.m_pResource = pResource;

    if (m_RequireLoading.IndexOf(li) == ezInvalidIndex)
    {
      // the resource is marked as 'preloading' but it is not in the queue anymore
      // that means some task is already working on loading it
      // therefore we should not touch it (especially unload it), it might end up in an inconsistent state

      ezLog::Dev("Resource '{0}' is not being reloaded, because it is currently loaded already", pResource->GetResourceID());
      return false;
    }
  }

  // if bForce, skip the outdated check
  if (!bForce)
  {
    if (!pLoader->IsResourceOutdated(pResource))
      return false;

    if (pResource->GetLoadingState() == ezResourceState::LoadedResourceMissing)
    {
      ezLog::Dev("Resource '{0}' is missing and will be tried to be reloaded ('{1}')", pResource->GetResourceID(), pResource->GetResourceDescription());
    }
    else
    {
      ezLog::Dev("Resource '{0}' is outdated and will be reloaded ('{1}')", pResource->GetResourceID(), pResource->GetResourceDescription());
    }
  }

  // make sure existing data is purged
  pResource->CallUnloadData(ezResourceBase::Unload::AllQualityLevels);

  EZ_ASSERT_DEV(pResource->GetLoadingState() <= ezResourceState::UnloadedMetaInfoAvailable, "Resource '{0}' should be in an unloaded state now.", pResource->GetResourceID());

  if (bAllowPreloading)
  {
    const ezTime tNow = m_LastFrameUpdate;

    // resources that have been in use recently will be put into the preload queue immediately
    // everything else will be loaded on demand
    if (pResource->GetLastAcquireTime() >= tNow - ezTime::Seconds(30.0))
    {
      // this will deadlock fmod soundbank loading
      // what happens is that PreloadResource sets the "IsPreloading" flag, because the soundbank is now in the queue
      // in case a soundevent is needed right away (very likely), to load that soundevent, the soundbank is needed, so the soundevent loader blocks until the soundbank is loaded
      // however, both loaders would currently run on the single "loading thread", so now the loading thread will wait for itself to finish, which never happens
      // instead, it SHOULD just load the soundbank itself, which is theoretically implemented, but does not happen when the "IsPreloading" flag is already set
      // there are multiple solutions
      // 1. do not depend on other resources while loading a resource, though this does not work for fmod soundevents
      // 2. trigger the 'bDoItYourself' code path above when on the loading thread, this would require InternalPreloadResource to somehow change
      // 3. move the soundevent loader off the loading thread, ie. by finally implementing ezResourceFlags::NoFileAccessRequired

      //ezLog::Info("Preloading resource: {0} ({1})", pResource->GetResourceID(), pResource->GetResourceDescription());
      //PreloadResource(pResource, tNow - pResource->GetLastAcquireTime());
    }
  }

  return true;
}

ezUInt32 ezResourceManager::ReloadResourcesOfType(const ezRTTI* pType, bool bForce)
{
  EZ_LOCK(s_ResourceMutex);
  EZ_LOG_BLOCK("ezResourceManager::ReloadResourcesOfType", pType->GetTypeName());

  ezUInt32 count = 0;

  for (auto it = m_LoadedResources.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetDynamicRTTI() == pType)
    {
      if (ReloadResource(it.Value(), bForce))
        ++count;
    }
  }

  return count;
}

// To allow triggering this event without a link dependency
// Used by Fileserve, to trigger this event, even though Fileserve should not have a link dependency on Core
EZ_ON_GLOBAL_EVENT(ezResourceManager_ReloadAllResources)
{
  ezResourceManager::ReloadAllResources(false);
}

ezUInt32 ezResourceManager::ReloadAllResources(bool bForce)
{
  EZ_LOCK(s_ResourceMutex);
  EZ_LOG_BLOCK("ezResourceManager::ReloadAllResources");

  ezUInt32 count = 0;

  for (auto it = m_LoadedResources.GetIterator(); it.IsValid(); ++it)
  {
    if (ReloadResource(it.Value(), bForce))
      ++count;
  }

  if (count > 0)
  {
    ezResourceManagerEvent e;
    e.m_EventType = ezResourceManagerEventType::ReloadAllResources;

    s_ManagerEvents.Broadcast(e);
  }

  return count;
}


void ezResourceManager::ResetAllResources()
{
  EZ_LOCK(s_ResourceMutex);
  EZ_LOG_BLOCK("ezResourceManager::ReloadAllResources");

  for (auto it = m_LoadedResources.GetIterator(); it.IsValid(); ++it)
  {
    ezResourceBase* pResource = it.Value();
    pResource->ResetResource();
  }

}

void ezResourceManager::PerFrameUpdate()
{
  m_LastFrameUpdate = ezTime::Now();

  if (m_bBroadcastExistsEvent)
  {
    EZ_LOCK(s_ResourceMutex);

    m_bBroadcastExistsEvent = false;

    for (auto it = m_LoadedResources.GetIterator(); it.IsValid(); ++it)
    {
      ezResourceEvent e;
      e.m_EventType = ezResourceEventType::ResourceExists;
      e.m_pResource = it.Value();

      ezResourceManager::BroadcastResourceEvent(e);
    }
  }
}

void ezResourceManager::BroadcastExistsEvent()
{
  m_bBroadcastExistsEvent = true;
}

void ezResourceManager::ConfigureResourceCategory(const char* szCategoryName, ezUInt64 uiMemoryLimitCPU, ezUInt64 uiMemoryLimitGPU)
{
  EZ_LOCK(s_ResourceMutex);

  ezTempHashedString sHash(szCategoryName);

  auto& cat = m_ResourceCategories[sHash.GetHash()];

  cat.m_sName = szCategoryName;
  cat.m_uiMemoryLimitCPU = uiMemoryLimitCPU;
  cat.m_uiMemoryLimitGPU = uiMemoryLimitGPU;

  ezResourceManagerEvent e;
  e.m_EventType = ezResourceManagerEventType::ResourceCategoryChanged;
  e.m_pCategory = &cat;

  s_ManagerEvents.Broadcast(e);
}

const ezResourceCategory& ezResourceManager::GetResourceCategory(const char* szCategoryName)
{
  ezTempHashedString sHash(szCategoryName);

  ezResourceCategory* pCat = nullptr;
  EZ_VERIFY(m_ResourceCategories.TryGetValue(sHash.GetHash(), pCat), "Resource Category '{0}' does not exist", szCategoryName);

  return *pCat;
}

/* Not yet good enough for prime time
void ezResourceManager::CleanUpResources()
{
  /// \todo Parameter to tweak cleanup time

  const ezTime tNow = ezTime::Now();

  /// \todo Not so often
  {
    static ezTime tLastCleanup;

    if (tNow - tLastCleanup < ezTime::Seconds(0.1))
      return;

    tLastCleanup = tNow;
  }

  /// \todo Lock ?
  EZ_LOCK(s_ResourceMutex);

  for (auto it = m_LoadedResources.GetIterator(); it.IsValid();)
  {
    ezResourceBase* pReference = it.Value();

    if (pReference->m_iReferenceCount == 0)
    {
      const auto& CurKey = it.Key();

      pReference->CallUnloadData(true);
      const ezRTTI* pRtti = pReference->GetDynamicRTTI();

      pRtti->GetAllocator()->Deallocate(pReference);

      ++it;

      m_LoadedResources.Remove(CurKey);
    }
    else
    {
      /// \todo Don't remove resources unless memory threshold is reached
      /// \todo virtual method on resource to query unload time

      ezTime LastAccess = pReference->m_LastAcquire;

      if (pReference->m_uiMaxQualityLevel > 0)
      {
        float fFactor = 1.0f - ((float) pReference->m_uiLoadedQualityLevel / ((float) pReference->m_uiMaxQualityLevel + 1.0f));
        LastAccess += fFactor * ezTime::Seconds(10.0);
      }

      LastAccess += ((5 - pReference->GetPriority()) * ezTime::Seconds(5.0));


      if (LastAccess < tNow)
      {
          if ((pReference->m_uiLoadedQualityLevel > 1) ||
              (pReference->m_uiLoadedQualityLevel > 0 && pReference->GetBaseResourceFlags().IsAnySet(ezResourceFlags::ResourceHasFallback)))
          {
            pReference->CallUnloadData(false);
            pReference->UpdateMemoryUsage();
          }

        ++it;
      }
      else
      {
        ++it;
      }
    }
  }
}
*/


void ezResourceManager::PluginEventHandler(const ezPlugin::PluginEvent& e)
{
  switch (e.m_EventType)
  {
  case ezPlugin::PluginEvent::AfterStartupShutdown:
    {
      // unload all resources until there are no more that can be unloaded
      // this is to prevent having resources allocated that came from a dynamic plugin
      FreeUnusedResources(true);
    }
    break;

  default:
    break;
  }
}

void ezResourceManager::OnCoreStartup()
{
  EZ_LOCK(s_ResourceMutex);
  m_bTaskRunning = false;
  m_bStop = false;

  ezPlugin::s_PluginEvents.AddEventHandler(PluginEventHandler);
}

void ezResourceManager::EngineAboutToShutdown()
{
  {
    EZ_LOCK(s_ResourceMutex);
    m_RequireLoading.Clear();
    m_bTaskRunning = true;
    m_bStop = true;
  }

  for (int i = 0; i < ezResourceManager::MaxDiskReadTasks; ++i)
  {
    ezTaskSystem::CancelTask(&m_WorkerTasksDiskRead[i]);
  }

  for (int i = 0; i < ezResourceManager::MaxMainThreadTasks; ++i)
  {
    ezTaskSystem::CancelTask(&m_WorkerTasksMainThread[i]);
  }
}

void ezResourceManager::OnEngineShutdown()
{
  ezResourceManagerEvent e;
  e.m_EventType = ezResourceManagerEventType::ManagerShuttingDown;

  // in case of a crash inside the event broadcast, you might have a resource type added through a dynamic plugin
  // that has already been unloaded, but the event handler is still referenced
  // to fix this, call CleanupDynamicPluginReferences() on that resource type during engine shutdown
  s_ManagerEvents.Broadcast(e);

  EngineAboutToShutdown();

  // unload all resources until there are no more that can be unloaded
  FreeUnusedResources(true);
}

void ezResourceManager::OnCoreShutdown()
{
  OnEngineShutdown();

  if (!m_LoadedResources.IsEmpty())
  {
    EZ_LOG_BLOCK("Referenced Resources");

    ezLog::Error("There are {0} resource still referenced.", m_LoadedResources.GetCount());

    for (auto it = m_LoadedResources.GetIterator(); it.IsValid(); ++it)
    {
      ezResourceBase* pReference = it.Value();

      ezLog::Info("Refcount = {0}, Type = '{1}', ResourceID = '{2}'", pReference->GetReferenceCount(), pReference->GetDynamicRTTI()->GetTypeName(), pReference->GetResourceID());
    }
  }

  ezPlugin::s_PluginEvents.RemoveEventHandler(PluginEventHandler);
}

ezResourceBase* ezResourceManager::GetResource(const ezRTTI* pRtti, const char* szResourceID, bool bIsReloadable)
{
  if (ezStringUtils::IsNullOrEmpty(szResourceID))
    return nullptr;

  ezResourceBase* pResource = nullptr;

  ezTempHashedString sResourceHash(szResourceID);

  EZ_LOCK(s_ResourceMutex);

  ezHashedString* redirection;
  if (s_NamedResources.TryGetValue(sResourceHash, redirection))
  {
    sResourceHash = *redirection;
    szResourceID = redirection->GetData();
  }

  if (m_LoadedResources.TryGetValue(sResourceHash, pResource))
  {
    EZ_ASSERT_DEBUG(pResource->GetDynamicRTTI() == pRtti, "Resource is not of expected type");
    return pResource;
  }

  EZ_ASSERT_DEV(pRtti != nullptr, "There is no RTTI information available for the given resource type '{0}'", EZ_STRINGIZE(ResourceType));
  EZ_ASSERT_DEV(pRtti->GetAllocator() != nullptr, "There is no RTTI allocator available for the given resource type '{0}'", EZ_STRINGIZE(ResourceType));

  ezResourceBase* pNewResource = pRtti->GetAllocator()->Allocate<ezResourceBase>();
  pNewResource->SetUniqueID(szResourceID, bIsReloadable);

  m_LoadedResources.Insert(sResourceHash, pNewResource);

  return pNewResource;
}

ezTypelessResourceHandle ezResourceManager::LoadResourceByType(const ezRTTI* pResourceType, const char* szResourceID)
{
  return ezTypelessResourceHandle(GetResource(pResourceType, szResourceID, true));
}

void ezResourceManager::RegisterNamedResource(const char* szLookupName, const char* szRedirectionResource)
{
  EZ_LOCK(s_ResourceMutex);

  ezTempHashedString lookup(szLookupName);

  ezHashedString redirection;
  redirection.Assign(szRedirectionResource);

  s_NamedResources[lookup] = redirection;
}

void ezResourceManager::UnregisterNamedResource(const char* szLookupName)
{
  EZ_LOCK(s_ResourceMutex);

  ezTempHashedString hash(szLookupName);
  s_NamedResources.Remove(hash);
}


void ezResourceManager::UpdateResourceWithCustomLoader(const ezTypelessResourceHandle& hResource, ezUniquePtr<ezResourceTypeLoader>&& loader)
{
  EZ_LOCK(s_ResourceMutex);

  hResource.m_pResource->m_Flags.Add(ezResourceFlags::HasCustomDataLoader);
  s_CustomLoaders[hResource.m_pResource] = std::move(loader);
  // if there was already a custom loader set, but it got no action yet, it is deleted here and replaced with the newer loader

  ReloadResource(hResource.m_pResource, true);
};

bool ezResourceManager::FinishLoadingOfResources()
{
  while (true)
  {
    ezUInt32 uiRemaining = 0;

    {
      EZ_LOCK(s_ResourceMutex);

      uiRemaining = m_RequireLoading.GetCount();
    }

    //if (uiRemaining == 0 && s_ResourcesInLoadingLimbo != 0)
    //{
      //ezUInt32 i = s_ResourcesInLoadingLimbo;
      //ezLog::Dev("Waiting for {0} resources on main thread", i);
    //}

    if (uiRemaining == 0 && s_ResourcesInLoadingLimbo == 0)
    {
      const bool bLoadedAny = s_ResourcesLoadedRecently.Set(0) > 0;
      //ezLog::Debug("FinishLoadingOfResources: {0}", bLoadedAny ? "true" : "false");
      return bLoadedAny;
    }

    HelpResourceLoading();
  }
}

void ezResourceManager::ClearAllResourceFallbacks()
{
  ezResourceManagerEvent e;
  e.m_EventType = ezResourceManagerEventType::ClearResourceFallbacks;
  s_ManagerEvents.Broadcast(e);
}

void ezResourceManager::EnsureResourceLoadingState(ezResourceBase* pResource, const ezResourceState RequestedState)
{
  // help loading until the requested resource is available
  while ((ezInt32)pResource->GetLoadingState() < (ezInt32)RequestedState && (pResource->GetLoadingState() != ezResourceState::LoadedResourceMissing))
  {
    HelpResourceLoading();
  }
}

bool ezResourceManager::HelpResourceLoading()
{
  if (!m_WorkerTasksDiskRead[m_iCurrentWorkerDiskRead].IsTaskFinished())
  {
    ezTaskSystem::WaitForTask(&m_WorkerTasksDiskRead[m_iCurrentWorkerDiskRead]);
    return true;
  }
  else
  {
    for (ezInt32 i = 0; i < MaxMainThreadTasks; ++i)
    {
      // get the 'oldest' main thread task in the queue and try to finish that first
      const ezInt32 iWorkerMainThread = (ezResourceManager::m_iCurrentWorkerMainThread + i) % MaxMainThreadTasks;

      if (!m_WorkerTasksMainThread[iWorkerMainThread].IsTaskFinished())
      {
        ezTaskSystem::WaitForTask(&m_WorkerTasksMainThread[iWorkerMainThread]);
        return true; // we waited for one of them, that's enough for this round
      }
    }
  }

  return false;
}

void ezResourceManager::SetResourceLowResData(const ezTypelessResourceHandle& hResource, ezStreamReader* pStream)
{
  ezResourceBase* pResource = hResource.m_pResource;

  if (pResource->GetBaseResourceFlags().IsSet(ezResourceFlags::HasLowResData))
    return;

  if (!pResource->GetBaseResourceFlags().IsSet(ezResourceFlags::IsReloadable))
    return;

  EZ_LOCK(s_ResourceMutex);

  // set this, even if we don't end up using the data (because some thread is already loading the full thing)
  pResource->m_Flags.Add(ezResourceFlags::HasLowResData);

  if (pResource->GetBaseResourceFlags().IsSet(ezResourceFlags::IsPreloading))
  {
    LoadingInfo li;
    li.m_pResource = pResource;

    if (!m_RequireLoading.Remove(li))
    {
      // if we cannot find it in the queue anymore, some thread already started loading it
      // in this case, do not try to modify it
      return;
    }

    pResource->m_Flags.Remove(ezResourceFlags::IsPreloading);
  }

  pResource->CallUpdateContent(pStream);

  EZ_ASSERT_DEV(pResource->GetLoadingState() != ezResourceState::Unloaded, "The resource should have changed its loading state.");

  // Update Memory Usage
  {
    ezResourceBase::MemoryUsage MemUsage;
    MemUsage.m_uiMemoryCPU = 0xFFFFFFFF;
    MemUsage.m_uiMemoryGPU = 0xFFFFFFFF;
    pResource->UpdateMemoryUsage(MemUsage);

    EZ_ASSERT_DEV(MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its CPU memory usage", pResource->GetResourceID());
    EZ_ASSERT_DEV(MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its GPU memory usage", pResource->GetResourceID());

    pResource->m_MemoryUsage = MemUsage;
  }
}

EZ_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_ResourceManager);

