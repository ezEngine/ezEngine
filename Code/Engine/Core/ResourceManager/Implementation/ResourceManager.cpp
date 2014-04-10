#include <Core/PCH.h>
#include <Core/ResourceManager/ResourceManager.h>

ezHashTable<ezResourceID, ezResourceBase*> ezResourceManager::m_LoadedResources;
ezMap<ezString, ezResourceTypeLoader*> ezResourceManager::m_ResourceTypeLoader;
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

void ezResourceManager::PreloadResource(ezResourceBase* pResource, bool bHighestPriority)
{
  // TODO: Make this publicly available

  if (m_bStop)
    return;

  ezLock<ezMutex> l(ResourceMutex);

  if (pResource->m_bIsPreloading)
    return;

  EZ_ASSERT(!pResource->m_bIsPreloading, "");
  pResource->m_bIsPreloading = true;
  EZ_ASSERT(pResource->m_bIsPreloading, "");

  LoadingInfo li;
  li.m_pResource = pResource;
  // not necessary here
  //li.m_DueDate = pResource->GetLoadingDeadline();

  if (bHighestPriority)
    m_RequireLoading.PushBack(li);
  else
    m_RequireLoading.PushFront(li);

  RunWorkerTask();
}

void ezResourceManager::RunWorkerTask()
{
  if (m_bStop)
    return;

  ezLock<ezMutex> l(ResourceMutex);

  if (!m_bTaskRunning && !ezResourceManager::m_RequireLoading.IsEmpty())
  {
    m_bTaskRunning = true;
    m_iCurrentWorker = (m_iCurrentWorker + 1) % 2;
    ezTaskSystem::StartSingleTask(&m_WorkerTask[m_iCurrentWorker], ezTaskPriority::FileAccess);
  }
}

void ezResourceManager::UpdateLoadingDeadlines()
{
  // TODO: don't do this too often

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastDeadLineUpdate < ezTime::Milliseconds(100))
    return;

  m_LastDeadLineUpdate = tNow;

  // TODO: Allow to tweak kick out time
  // TODO: Make sure resources that are queued here don't get deleted

  const ezTime tKickOut = tNow + ezTime::Seconds(30.0);

  ezUInt32 uiCount = m_RequireLoading.GetCount();
  for (ezUInt32 i = 0; i < uiCount; )
  {
    m_RequireLoading[i].m_DueDate = m_RequireLoading[i].m_pResource->GetLoadingDeadline(tNow);

    if (m_RequireLoading[i].m_DueDate > tKickOut)
    {
      EZ_ASSERT(m_RequireLoading[i].m_pResource->m_bIsPreloading == true, "");
      m_RequireLoading[i].m_pResource->m_bIsPreloading = false;
      EZ_ASSERT(m_RequireLoading[i].m_pResource->m_bIsPreloading == false, "");

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
  m_pResourceToLoad->UpdateContent(*m_LoaderData.m_pDataStream);
  m_pResourceToLoad->UpdateMemoryUsage();

  m_pLoader->CloseDataStream(m_pResourceToLoad, m_LoaderData);

  {
    ezLock<ezMutex> l(ResourceMutex);
    EZ_ASSERT(m_pResourceToLoad->m_bIsPreloading == true, "");
    m_pResourceToLoad->m_bIsPreloading = false;
    EZ_ASSERT(m_pResourceToLoad->m_bIsPreloading == false, "");
  }

  m_pLoader = NULL;
  m_pResourceToLoad = NULL;
}

void ezResourceManagerWorker::Execute()
{
  ezResourceBase* pResourceToLoad = NULL;

  {
    ezLock<ezMutex> l(ResourceMutex);

    ezResourceManager::UpdateLoadingDeadlines();

    if (ezResourceManager::m_RequireLoading.IsEmpty())
    {
      ezResourceManager::m_bTaskRunning = false;
      return;
    }

    auto it = ezResourceManager::m_RequireLoading.PeekBack();
    pResourceToLoad = it.m_pResource;
    ezResourceManager::m_RequireLoading.PopBack();
  }

  const ezResourceLoadState::Enum CurState = pResourceToLoad->GetLoadingState();

  ezResourceTypeLoader* pLoader = ezResourceManager::GetResourceTypeLoader(pResourceToLoad->GetDynamicRTTI());

  EZ_ASSERT(pLoader != NULL, "No Loader function available for Resource Type '%s'", pResourceToLoad->GetDynamicRTTI()->GetTypeName());

  ezResourceLoadData LoaderData = pLoader->OpenDataStream(pResourceToLoad);

  // the resource data has been loaded (at least one piece), reset the due date
  pResourceToLoad->SetDueDate();

  bool bResourceIsPreloading = false;

  if (pResourceToLoad->GetResourceFlags().IsAnySet(ezResourceFlags::UpdateOnMainThread))
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
    pResourceToLoad->UpdateContent(*LoaderData.m_pDataStream);
    pResourceToLoad->UpdateMemoryUsage();

    /// \todo Proper cleanup
    //EZ_DEFAULT_DELETE(pStream);
    pLoader->CloseDataStream(pResourceToLoad, LoaderData);
  }


  {
    ezLock<ezMutex> l(ResourceMutex);

    if (!bResourceIsPreloading)
    {
      EZ_ASSERT(pResourceToLoad->m_bIsPreloading == true, "");
      pResourceToLoad->m_bIsPreloading = false;
      EZ_ASSERT(pResourceToLoad->m_bIsPreloading == false, "");
    }

    ezResourceManager::m_bTaskRunning = false;
    ezResourceManager::RunWorkerTask();
  }
}

void ezResourceManager::CleanUpResources()
{
  // TODO: Parameter to tweak cleanup time

  const ezTime tNow = ezTime::Now();

  // TODO: Not so often
  {
    static ezTime tLastCleanup;

    if (tNow - tLastCleanup < ezTime::Seconds(0.1))
      return;

    tLastCleanup = tNow;
  }

  // TODO: Lock ?
  ezLock<ezMutex> l(ResourceMutex);

  for (auto it = m_LoadedResources.GetIterator(); it.IsValid();)
  {
    ezResourceBase* pReference = it.Value();

    if (pReference->m_iReferenceCount == 0)
    {
      const auto& CurKey = it.Key();

      pReference->UnloadData(true);
      EZ_DEFAULT_DELETE(pReference);

      ++it;

      m_LoadedResources.Remove(CurKey);
    }
    else
    {
      // TODO: Don't remove resources unless memory threshold is reached
      // TODO: virtual method on resource to query unload time

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
              (pReference->m_uiLoadedQualityLevel > 0 && pReference->m_bHasFallback))
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

void ezResourceManager::Shutdown()
{
  {
    ezLock<ezMutex> l(ResourceMutex);
    m_RequireLoading.Clear();
    m_bTaskRunning = true;
    m_bStop = true;
  }

  for (int i = 0; i < 16; ++i)
  {
    ezTaskSystem::CancelTask(&m_WorkerGPU[i]);
  }

  for (int i = 0; i < 2; ++i)
  {
    ezTaskSystem::CancelTask(&m_WorkerTask[i]);
  }
}

