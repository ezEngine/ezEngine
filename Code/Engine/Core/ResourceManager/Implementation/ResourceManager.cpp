#include <Core/PCH.h>
#include <Core/ResourceManager/ResourceManager.h>

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
ezResourceManagerWorker ezResourceManager::m_WorkerTask[2];
ezResourceManagerWorkerGPU ezResourceManager::m_WorkerGPU[16];
ezInt8 ezResourceManager::m_iCurrentWorkerGPU = 0;
ezInt8 ezResourceManager::m_iCurrentWorker = 0;
ezTime ezResourceManager::m_LastDeadLineUpdate;
ezTime ezResourceManager::m_LastFrameUpdate;
bool ezResourceManager::m_bBroadcastExistsEvent = false;
ezHashTable<ezUInt32, ezResourceManager::ResourceCategory> ezResourceManager::m_ResourceCategories;
ezEvent<const ezResourceManager::ResourceEvent&> ezResourceManager::s_ResourceEvents;
ezEvent<const ezResourceManager::ManagerEvent&> ezResourceManager::s_ManagerEvents;

ezResourceTypeLoader* ezResourceManager::GetResourceTypeLoader(const ezRTTI* pRTTI)
{
  return m_ResourceTypeLoader[pRTTI->GetTypeName()];
}

ezMutex ResourceMutex;

void ezResourceManager::BroadcastResourceEvent(const ResourceEvent& e)
{
  EZ_LOCK(ResourceMutex);

  s_ResourceEvents.Broadcast(e);
}

void ezResourceManager::InternalPreloadResource(ezResourceBase* pResource, bool bHighestPriority)
{
  if (m_bStop)
    return;

  EZ_LOCK(ResourceMutex);

  // if there is nothing else that could be loaded, just return right away
  if (pResource->GetLoadingState() == ezResourceState::Loaded && pResource->GetNumQualityLevelsLoadable() == 0)
    return;

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
        m_RequireLoading.PushFront(li);
    }

    return;
  }

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

  ezResourceManager::ResourceEvent e;
  e.m_pResource = pResource;
  e.m_EventType = ezResourceManager::ResourceEventType::ResourceInPreloadQueue;
  ezResourceManager::BroadcastResourceEvent(e);

  RunWorkerTask();
}

void ezResourceManager::RunWorkerTask()
{
  if (m_bStop)
    return;

  EZ_LOCK(ResourceMutex);

  static bool bTaskNamesInitialized = false;

  if (!bTaskNamesInitialized)
  {
    bTaskNamesInitialized = true;

    m_WorkerTask[0].SetTaskName("Resource Loader 1");
    m_WorkerTask[1].SetTaskName("Resource Loader 2");

    ezStringBuilder s;
    for (ezUInt32 i = 0; i < 16; ++i)
    {
      s.Format("GPU Resource Loader %u", i);
      m_WorkerGPU[i].SetTaskName(s.GetData());
    }
  }

  if (!m_bTaskRunning && !ezResourceManager::m_RequireLoading.IsEmpty())
  {
    m_bTaskRunning = true;
    m_iCurrentWorker = (m_iCurrentWorker + 1) % 2;
    ezTaskSystem::StartSingleTask(&m_WorkerTask[m_iCurrentWorker], ezTaskPriority::FileAccess);
  }
}

void ezResourceManager::UpdateLoadingDeadlines()
{
  // we are already in the ResourceMutex here

  /// \todo don't do this too often

  const ezTime tNow = m_LastFrameUpdate;

  if (tNow - m_LastDeadLineUpdate < ezTime::Milliseconds(100))
    return;

  m_LastDeadLineUpdate = tNow;

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

      ezResourceManager::ResourceEvent e;
      e.m_pResource = m_RequireLoading[i].m_pResource;
      e.m_EventType = ezResourceManager::ResourceEventType::ResourceOutOfPreloadQueue;
      ezResourceManager::BroadcastResourceEvent(e);

      m_RequireLoading.RemoveAtSwap(i);
      --uiCount;
    }
    else
      ++i;
  }

  m_RequireLoading.Sort();
}

void ezResourceManagerWorkerGPU::Execute()
{
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

    EZ_ASSERT_DEV(MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '%s' did not properly update its CPU memory usage", m_pResourceToLoad->GetResourceID().GetData());
    EZ_ASSERT_DEV(MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '%s' did not properly update its GPU memory usage", m_pResourceToLoad->GetResourceID().GetData());

    m_pResourceToLoad->m_MemoryUsage = MemUsage;
  }

  m_pLoader->CloseDataStream(m_pResourceToLoad, m_LoaderData);

  {
    EZ_LOCK(ResourceMutex);
    EZ_ASSERT_DEV(m_pResourceToLoad->m_Flags.IsSet(ezResourceFlags::IsPreloading) == true, "");
    m_pResourceToLoad->m_Flags.Remove(ezResourceFlags::IsPreloading);

    ezResourceManager::ResourceEvent e;
    e.m_pResource = m_pResourceToLoad;
    e.m_EventType = ezResourceManager::ResourceEventType::ResourceOutOfPreloadQueue;
    ezResourceManager::BroadcastResourceEvent(e);
  }

  m_pLoader = NULL;
  m_pResourceToLoad = NULL;
}

void ezResourceManagerWorker::Execute()
{
  ezResourceBase* pResourceToLoad = NULL;

  {
    EZ_LOCK(ResourceMutex);

    ezResourceManager::UpdateLoadingDeadlines();

    if (ezResourceManager::m_RequireLoading.IsEmpty())
    {
      ezResourceManager::m_bTaskRunning = false;
      return;
    }

    auto it = ezResourceManager::m_RequireLoading.PeekFront();
    pResourceToLoad = it.m_pResource;
    ezResourceManager::m_RequireLoading.PopFront();
  }

  ezResourceTypeLoader* pLoader = ezResourceManager::GetResourceTypeLoader(pResourceToLoad->GetDynamicRTTI());

  if (pLoader == NULL)
    pLoader = pResourceToLoad->GetDefaultResourceTypeLoader();

  EZ_ASSERT_DEV(pLoader != NULL, "No Loader function available for Resource Type '%s'", pResourceToLoad->GetDynamicRTTI()->GetTypeName());

  ezResourceLoadData LoaderData = pLoader->OpenDataStream(pResourceToLoad);

  // the resource data has been loaded (at least one piece), reset the due date
  pResourceToLoad->SetDueDate();

  bool bResourceIsPreloading = false;

  if (pResourceToLoad->GetBaseResourceFlags().IsAnySet(ezResourceFlags::UpdateOnMainThread))
  {
    bResourceIsPreloading = true;

    ezResourceManagerWorkerGPU* pWorkerGPU = &ezResourceManager::m_WorkerGPU[ezResourceManager::m_iCurrentWorkerGPU];
    ezResourceManager::m_iCurrentWorkerGPU = (ezResourceManager::m_iCurrentWorkerGPU + 1) % 16;

    ezTaskSystem::WaitForTask(pWorkerGPU);

    pWorkerGPU->m_LoaderData = LoaderData;
    pWorkerGPU->m_pLoader = pLoader;
    pWorkerGPU->m_pResourceToLoad = pResourceToLoad;

    ezTaskSystem::StartSingleTask(pWorkerGPU, ezTaskPriority::SomeFrameMainThread);
  }
  else
  {
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

      EZ_ASSERT_DEV(MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '%s' did not properly update its CPU memory usage", pResourceToLoad->GetResourceID().GetData());
      EZ_ASSERT_DEV(MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '%s' did not properly update its GPU memory usage", pResourceToLoad->GetResourceID().GetData());

      pResourceToLoad->m_MemoryUsage = MemUsage;
    }

    /// \todo Proper cleanup
    //EZ_DEFAULT_DELETE(pStream);
    pLoader->CloseDataStream(pResourceToLoad, LoaderData);
  }


  {
    EZ_LOCK(ResourceMutex);

    if (!bResourceIsPreloading)
    {
      EZ_ASSERT_DEV(pResourceToLoad->m_Flags.IsSet(ezResourceFlags::IsPreloading) == true, "");
      pResourceToLoad->m_Flags.Remove(ezResourceFlags::IsPreloading);

      ezResourceManager::ResourceEvent e;
      e.m_pResource = pResourceToLoad;
      e.m_EventType = ezResourceManager::ResourceEventType::ResourceOutOfPreloadQueue;
      ezResourceManager::BroadcastResourceEvent(e);
    }

    ezResourceManager::m_bTaskRunning = false;
    ezResourceManager::RunWorkerTask();
  }
}

ezUInt32 ezResourceManager::FreeUnusedResources(bool bFreeAllUnused)
{
  EZ_LOCK(ResourceMutex);

  ezUInt32 uiUnloaded = 0;
  bool bUnloadedAny = false;

  do
  {
    bUnloadedAny =false;

    for (auto it = m_LoadedResources.GetIterator(); it.IsValid(); /* empty */)
    {
      ezResourceBase* pReference = it.Value();

      if (pReference->m_iReferenceCount > 0)
      {
        ++it;
        continue;
      }

      const auto& CurKey = it.Key();

      EZ_ASSERT_DEV(pReference->m_iLockCount == 0, "Resource '%s' has a refcount of zero, but is still in an acquired state.", pReference->GetResourceID().GetData());

      bUnloadedAny = true;
      ++uiUnloaded;
      pReference->CallUnloadData(ezResourceBase::Unload::AllQualityLevels);

      EZ_ASSERT_DEV(pReference->GetLoadingState() <= ezResourceState::UnloadedMetaInfoAvailable, "Resource '%s' should be in an unloaded state now.", pReference->GetResourceID().GetData());

      // broadcast that we are going to delete the resource
      {
        ezResourceManager::ResourceEvent e;
        e.m_pResource = pReference;
        e.m_EventType = ezResourceManager::ResourceEventType::ResourceDeleted;
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


void ezResourceManager::ReloadResource(ezResourceBase* pResource)
{
  EZ_LOCK(ResourceMutex);

  if (!pResource->m_Flags.IsAnySet(ezResourceFlags::IsReloadable))
    return;

  ezResourceTypeLoader* pLoader = ezResourceManager::GetResourceTypeLoader(pResource->GetDynamicRTTI());

  if (pLoader == nullptr)
    pLoader = pResource->GetDefaultResourceTypeLoader();

  if (pLoader == nullptr)
    return;

  // no need to reload resources that are not loaded so far
  if (pResource->GetLoadingState() == ezResourceState::Unloaded)
    return;

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

      ezLog::Dev("Resource '%s' is not being reloaded, because it is currently loaded already", pResource->GetResourceID().GetData());
      return;
    }
  }

  if (pResource->GetLoadingState() != ezResourceState::LoadedResourceMissing)
  {
    if (!pLoader->IsResourceOutdated(pResource))
      return;

    ezLog::Dev("Resource '%s' is outdated and will be reloaded", pResource->GetResourceID().GetData());
  }
  else
  {
    ezLog::Dev("Resource '%s' is missing and will be tried to be reloaded", pResource->GetResourceID().GetData());
  }

  // make sure existing data is purged
  pResource->CallUnloadData(ezResourceBase::Unload::AllQualityLevels);

  EZ_ASSERT_DEV(pResource->GetLoadingState() <= ezResourceState::UnloadedMetaInfoAvailable, "Resource '%s' should be in an unloaded state now.", pResource->GetResourceID().GetData());

  if (bAllowPreloading)
  {
    const ezTime tNow = m_LastFrameUpdate;

    // resources that have been in use recently will be put into the preload queue immediately
    // everything else will be loaded on demand
    if (pResource->GetLastAcquireTime() >= tNow - ezTime::Seconds(30.0))
    {
      PreloadResource(pResource, tNow - pResource->GetLastAcquireTime());
    }
  }
}

void ezResourceManager::ReloadResourcesOfType(const ezRTTI* pType)
{
  EZ_LOCK(ResourceMutex);

  for (auto it = m_LoadedResources.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetDynamicRTTI() == pType)
      ReloadResource(it.Value());
  }
}

void ezResourceManager::ReloadAllResources()
{
  EZ_LOCK(ResourceMutex);

  for (auto it = m_LoadedResources.GetIterator(); it.IsValid(); ++it)
  {
    ReloadResource(it.Value());
  }
}

void ezResourceManager::PerFrameUpdate()
{
  m_LastFrameUpdate = ezTime::Now();

  if (m_bBroadcastExistsEvent)
  {
    EZ_LOCK(ResourceMutex);

    m_bBroadcastExistsEvent = false;

    for (auto it = m_LoadedResources.GetIterator(); it.IsValid(); ++it)
    {
      ezResourceManager::ResourceEvent e;
      e.m_EventType = ezResourceManager::ResourceEventType::ResourceExists;
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
  ezTempHashedString sHash(szCategoryName);

  auto& cat = m_ResourceCategories[sHash.GetHash()];

  cat.m_sName = szCategoryName;
  cat.m_uiMemoryLimitCPU = uiMemoryLimitCPU;
  cat.m_uiMemoryLimitGPU = uiMemoryLimitGPU;

  ManagerEvent e;
  e.m_EventType = ManagerEventType::ResourceCategoryChanged;
  e.m_pCategory = &cat;

  s_ManagerEvents.Broadcast(e);
}

const ezResourceManager::ResourceCategory& ezResourceManager::GetResourceCategory(const char* szCategoryName)
{
  ezTempHashedString sHash(szCategoryName);

  ResourceCategory* pCat = nullptr;
  EZ_VERIFY(m_ResourceCategories.TryGetValue(sHash.GetHash(), pCat), "Resource Category '%s' does not exist", szCategoryName);

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
  EZ_LOCK(ResourceMutex);

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

void ezResourceManager::OnCoreStartup()
{
  EZ_LOCK(ResourceMutex);
  m_bTaskRunning = false;
  m_bStop = false;
}

void ezResourceManager::OnEngineShutdown()
{
  ManagerEvent e;
  e.m_EventType = ManagerEventType::ManagerShuttingDown;
  s_ManagerEvents.Broadcast(e);

  {
    EZ_LOCK(ResourceMutex);
    m_RequireLoading.Clear();
    m_bTaskRunning = true;
    m_bStop = true;
  }

  for (int i = 0; i < 2; ++i)
  {
    ezTaskSystem::CancelTask(&m_WorkerTask[i]);
  }

  for (int i = 0; i < 16; ++i)
  {
    ezTaskSystem::CancelTask(&m_WorkerGPU[i]);
  }

  // unload all resources until there are no more that can be unloaded
  FreeUnusedResources(true);
}

void ezResourceManager::OnCoreShutdown()
{
  OnEngineShutdown();

  if (!m_LoadedResources.IsEmpty())
  {
    EZ_LOG_BLOCK("Referenced Resources");

    ezLog::Error("There are %i resource still referenced.", m_LoadedResources.GetCount());

    for (auto it = m_LoadedResources.GetIterator(); it.IsValid(); ++it)
    {
      ezResourceBase* pReference = it.Value();

      ezLog::Info("Refcount = %i, Type = '%s', ResourceID = '%s'", pReference->GetReferenceCount(), pReference->GetDynamicRTTI()->GetTypeName(), pReference->GetResourceID().GetData());
    }
  }
}

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
 
EZ_END_SUBSYSTEM_DECLARATION



EZ_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_ResourceManager);

