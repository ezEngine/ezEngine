#include <Core/PCH.h>
#include <Core/ResourceManager/ResourceManager.h>

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

ezResourceTypeLoader* ezResourceManager::GetResourceTypeLoader(const ezRTTI* pRTTI)
{
  return m_ResourceTypeLoader[pRTTI->GetTypeName()];
}

ezMutex ResourceMutex;

void ezResourceManager::InternalPreloadResource(ezResourceBase* pResource, bool bHighestPriority)
{
  if (m_bStop)
    return;

  EZ_LOCK(ResourceMutex);

  if (pResource->GetLoadingState() == ezResourceLoadState::Loaded && pResource->m_uiLoadedQualityLevel >= pResource->m_uiMaxQualityLevel)
    return;

  // if we are already preloading this resource, but now it has highest priority
  // and it is still in the task queue (so not yet started)
  if (pResource->m_bIsPreloading)
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

  EZ_ASSERT(!pResource->m_bIsPreloading, "");
  pResource->m_bIsPreloading = true;

  LoadingInfo li;
  li.m_pResource = pResource;
  // not necessary here
  //li.m_DueDate = pResource->GetLoadingDeadline();

  if (bHighestPriority)
    m_RequireLoading.PushFront(li);
  else
    m_RequireLoading.PushBack(li);

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
  /// \todo don't do this too often

  const ezTime tNow = ezTime::Now();

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
      EZ_ASSERT(m_RequireLoading[i].m_pResource->m_bIsPreloading == true, "");
      m_RequireLoading[i].m_pResource->m_bIsPreloading = false;

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
  m_pResourceToLoad->UpdateContent(m_LoaderData.m_pDataStream);

  // update the file modification date, if available
  if (m_LoaderData.m_LoadedFileModificationDate.IsValid())
    m_pResourceToLoad->m_LoadedFileModificationTime = m_LoaderData.m_LoadedFileModificationDate;

  EZ_ASSERT(m_pResourceToLoad->GetLoadingState() != ezResourceLoadState::Uninitialized, "The resource should have changed its loading state.");
  EZ_ASSERT(m_pResourceToLoad->GetMaxQualityLevel() > 0, "The resource should have changed its max quality level (at least to 1).");

  m_pResourceToLoad->UpdateMemoryUsage();

  m_pLoader->CloseDataStream(m_pResourceToLoad, m_LoaderData);

  {
    EZ_LOCK(ResourceMutex);
    EZ_ASSERT(m_pResourceToLoad->m_bIsPreloading == true, "");
    m_pResourceToLoad->m_bIsPreloading = false;
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

  const ezResourceLoadState::Enum CurState = pResourceToLoad->GetLoadingState();

  ezResourceTypeLoader* pLoader = ezResourceManager::GetResourceTypeLoader(pResourceToLoad->GetDynamicRTTI());

  if (pLoader == NULL)
    pLoader = pResourceToLoad->GetDefaultResourceTypeLoader();

  EZ_ASSERT(pLoader != NULL, "No Loader function available for Resource Type '%s'", pResourceToLoad->GetDynamicRTTI()->GetTypeName());

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
    pResourceToLoad->UpdateContent(LoaderData.m_pDataStream);

    // update the file modification date, if available
    if (LoaderData.m_LoadedFileModificationDate.IsValid())
      pResourceToLoad->m_LoadedFileModificationTime = LoaderData.m_LoadedFileModificationDate;

    EZ_ASSERT(pResourceToLoad->GetLoadingState() != ezResourceLoadState::Uninitialized, "The resource should have changed its loading state.");
    EZ_ASSERT(pResourceToLoad->GetMaxQualityLevel() > 0, "The resource should have changed its max quality level (at least to 1).");

    pResourceToLoad->UpdateMemoryUsage();

    /// \todo Proper cleanup
    //EZ_DEFAULT_DELETE(pStream);
    pLoader->CloseDataStream(pResourceToLoad, LoaderData);
  }


  {
    EZ_LOCK(ResourceMutex);

    if (!bResourceIsPreloading)
    {
      EZ_ASSERT(pResourceToLoad->m_bIsPreloading == true, "");
      pResourceToLoad->m_bIsPreloading = false;
    }

    ezResourceManager::m_bTaskRunning = false;
    ezResourceManager::RunWorkerTask();
  }
}

ezUInt32 ezResourceManager::FreeUnusedResources()
{
  EZ_LOCK(ResourceMutex);

  ezUInt32 uiUnloaded = 0;

  for (auto it = m_LoadedResources.GetIterator(); it.IsValid(); /* empty */)
  {
    ezResourceBase* pReference = it.Value();

    if (pReference->m_iReferenceCount > 0)
    {
      ++it;
      continue;
    }

    const auto& CurKey = it.Key();

    EZ_ASSERT(pReference->m_iLockCount == 0, "Resource '%s' has a refcount of zero, but is still in an acquired state.", pReference->GetResourceID().GetData());

    ++uiUnloaded;
    pReference->UnloadData(true);

    EZ_ASSERT(pReference->GetLoadingState() <= ezResourceLoadState::MetaInfoAvailable, "Resource '%s' should be in an unloaded state now.", pReference->GetResourceID().GetData());

    // delete the resource via the RTTI provided allocator
    pReference->GetDynamicRTTI()->GetAllocator()->Deallocate(pReference);

    ++it;

    m_LoadedResources.Remove(CurKey);
  }

  return uiUnloaded;
}

void ezResourceManager::PreloadResource(ezResourceBase* pResource, ezTime tShouldBeAvailableIn)
{
  const ezTime tNow = ezTime::Now();

  pResource->SetDueDate(ezMath::Min(tNow + tShouldBeAvailableIn, pResource->m_DueDate));
  InternalPreloadResource(pResource, tShouldBeAvailableIn <= ezTime::Seconds(0.0)); // if the user set the timeout to zero or below, it will be scheduled immediately
}


void ezResourceManager::ReloadResource(ezResourceBase* pResource)
{
  EZ_LOCK(ResourceMutex);

  if (pResource->m_Flags.IsAnySet(ezResourceFlags::WasCreated))
    return;

  ezResourceTypeLoader* pLoader = ezResourceManager::GetResourceTypeLoader(pResource->GetDynamicRTTI());

  if (pLoader == nullptr)
    pLoader = pResource->GetDefaultResourceTypeLoader();

  if (pLoader == nullptr)
    return;

  // no need to reload resources that are not loaded so far
  if (pResource->GetLoadingState() == ezResourceLoadState::Uninitialized)
    return;

  bool bAllowPreloading = true;

  // if the resource is already in the preloading queue we can just keep it there
  if (pResource->m_bIsPreloading)
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

  if (!pLoader->IsResourceOutdated(pResource))
    return;

  ezLog::Dev("Resource '%s' is outdated and will be reloaded", pResource->GetResourceID().GetData());

  // make sure existing data is purged
  pResource->UnloadData(true);

  EZ_ASSERT(pResource->GetLoadingState() <= ezResourceLoadState::MetaInfoAvailable, "Resource '%s' should be in an unloaded state now.", pResource->GetResourceID().GetData());

  if (bAllowPreloading)
  {
    const ezTime tNow = ezTime::Now();

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

      pReference->UnloadData(true);
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
            pReference->UnloadData(false);
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
  while (FreeUnusedResources() > 0) { /* empty */ }
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

