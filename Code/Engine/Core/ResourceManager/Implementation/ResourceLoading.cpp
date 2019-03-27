#include <CorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Profiling/Profiling.h>

ezTypelessResourceHandle ezResourceManager::LoadResourceByType(const ezRTTI* pResourceType, const char* szResourceID)
{
  return ezTypelessResourceHandle(GetResource(pResourceType, szResourceID, true));
}

void ezResourceManager::InternalPreloadResource(ezResource* pResource, bool bHighestPriority)
{
  if (s_bShutdown)
    return;

  EZ_PROFILE_SCOPE("InternalPreloadResource");

  EZ_LOCK(s_ResourceMutex);

  // if there is nothing else that could be loaded, just return right away
  if (pResource->GetLoadingState() == ezResourceState::Loaded && pResource->GetNumQualityLevelsLoadable() == 0)
  {
    // due to the threading this can happen for all resource types and is valid
    // EZ_ASSERT_DEV(!pResource->m_Flags.IsSet(ezResourceFlags::IsPreloading), "Invalid flag on resource type '{0}'",
    // pResource->GetDynamicRTTI()->GetTypeName());
    return;
  }

  EZ_ASSERT_DEV(!s_bExportMode, "Resources should not be loaded in export mode");

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
      if (s_RequireLoading.RemoveAndCopy(li))
      {
        s_RequireLoading.PushFront(li);
      }
    }

    return;
  }

  EZ_ASSERT_DEV(!pResource->m_Flags.IsSet(ezResourceFlags::IsPreloading), "");
  pResource->m_Flags.Add(ezResourceFlags::IsPreloading);

  LoadingInfo li;
  li.m_pResource = pResource;
  // not necessary here
  // li.m_DueDate = pResource->GetLoadingDeadline();

  if (bHighestPriority)
    s_RequireLoading.PushFront(li);
  else
    s_RequireLoading.PushBack(li);

  // the mutex will be released by RunWorkerTask
  RunWorkerTask(pResource);
}

void ezResourceManager::RunWorkerTask(ezResource* pResource)
{
  if (s_bShutdown)
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

      for (ezUInt32 i = 0; i < MaxDataLoadTasks; ++i)
      {
        s.Format("Resource Data Loader {0}", i);
        s_WorkerTasksDataLoad[i].SetTaskName(s.GetData());
      }

      for (ezUInt32 i = 0; i < MaxUpdateContentTasks; ++i)
      {
        s.Format("Resource Content Updater {0}", i);
        s_WorkerTasksUpdateContent[i].SetTaskName(s.GetData());
      }
    }

    if (pResource != nullptr && ezTaskSystem::IsLoadingThread())
    {
      bDoItYourself = true;
    }
    else if (!s_bTaskRunning && !ezResourceManager::s_RequireLoading.IsEmpty())
    {
      s_bTaskRunning = true;
      s_uiCurrentLoadDataWorkerTask = (s_uiCurrentLoadDataWorkerTask + 1) % MaxDataLoadTasks;
      ezTaskSystem::StartSingleTask(&s_WorkerTasksDataLoad[s_uiCurrentLoadDataWorkerTask], ezTaskPriority::FileAccess);
    }
  }

  if (bDoItYourself)
  {
    // this function is always called from within a mutex
    // but we need to release the mutex between every loop iteration to prevent deadlocks
    s_ResourceMutex.Release();

    while (true)
    {
      ezResourceManagerWorkerDataLoad::DoWork(true);

      {
        EZ_LOCK(s_ResourceMutex);

        if (!pResource->m_Flags.IsAnySet(ezResourceFlags::IsPreloading))
        {
          break;
        }
      }
    }

    // reacquire to get into the proper state
    s_ResourceMutex.Acquire();
  }
}

void ezResourceManager::UpdateLoadingDeadlines()
{
  // EZ_ASSERT_DEBUG(s_ResourceMutex.IsLocked(), "Calling code must acquire s_ResourceMutex");

  /// \todo do this incrementally each frame

  const ezTime tNow = ezTime::Now();

  if (tNow - s_LastDeadlineUpdate < ezTime::Milliseconds(100))
    return;

  s_LastDeadlineUpdate = tNow;

  if (s_RequireLoading.IsEmpty())
    return;

  EZ_PROFILE_SCOPE("UpdateLoadingDeadlines");

  ezUInt32 uiCount = s_RequireLoading.GetCount();
  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    s_RequireLoading[i].m_DueDate = s_RequireLoading[i].m_pResource->GetLoadingDeadline(tNow);
  }

  s_RequireLoading.Sort();
}


void ezResourceManager::PreloadResource(ezResource* pResource, ezTime tShouldBeAvailableIn)
{
  const ezTime tNow = s_LastFrameUpdate;

  pResource->SetDueDate(ezMath::Min(tNow + tShouldBeAvailableIn, pResource->m_DueDate));
  InternalPreloadResource(pResource,
    tShouldBeAvailableIn <= ezTime::Seconds(0.0)); // if the user set the timeout to zero or below, it will be scheduled immediately
}


void ezResourceManager::PreloadResource(const ezTypelessResourceHandle& hResource, ezTime tShouldBeAvailableIn)
{
  // this is the same as BeginAcquireResource in PointerOnly mode

  EZ_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

  ezResource* pResource = hResource.m_pResource;
  EZ_ASSERT_DEV(
    pResource->m_iLockCount < 20, "You probably forgot somewhere to call 'EndAcquireResource' in sync with 'BeginAcquireResource'.");

  {
    pResource->m_iLockCount.Increment();
    PreloadResource(hResource.m_pResource, tShouldBeAvailableIn);
    pResource->m_iLockCount.Decrement();
  }
}

bool ezResourceManager::ReloadResource(ezResource* pResource, bool bForce)
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

    if (s_RequireLoading.IndexOf(li) == ezInvalidIndex)
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
      ezLog::Dev("Resource '{0}' is missing and will be tried to be reloaded ('{1}')", pResource->GetResourceID(),
        pResource->GetResourceDescription());
    }
    else
    {
      ezLog::Dev(
        "Resource '{0}' is outdated and will be reloaded ('{1}')", pResource->GetResourceID(), pResource->GetResourceDescription());
    }
  }

  // make sure existing data is purged
  pResource->CallUnloadData(ezResource::Unload::AllQualityLevels);

  EZ_ASSERT_DEV(pResource->GetLoadingState() <= ezResourceState::UnloadedMetaInfoAvailable,
    "Resource '{0}' should be in an unloaded state now.", pResource->GetResourceID());

  if (bAllowPreloading)
  {
    const ezTime tNow = s_LastFrameUpdate;

    // resources that have been in use recently will be put into the preload queue immediately
    // everything else will be loaded on demand
    if (pResource->GetLastAcquireTime() >= tNow - ezTime::Seconds(30.0))
    {
      // this will deadlock fmod soundbank loading
      // what happens is that PreloadResource sets the "IsPreloading" flag, because the soundbank is now in the queue
      // in case a soundevent is needed right away (very likely), to load that soundevent, the soundbank is needed, so the soundevent loader
      // blocks until the soundbank is loaded however, both loaders would currently run on the single "loading thread", so now the loading
      // thread will wait for itself to finish, which never happens instead, it SHOULD just load the soundbank itself, which is
      // theoretically implemented, but does not happen when the "IsPreloading" flag is already set there are multiple solutions
      // 1. do not depend on other resources while loading a resource, though this does not work for fmod soundevents
      // 2. trigger the 'bDoItYourself' code path above when on the loading thread, this would require InternalPreloadResource to somehow
      // change
      // 3. move the soundevent loader off the loading thread, ie. by finally implementing ezResourceFlags::NoFileAccessRequired

      // PreloadResource(pResource, tNow - pResource->GetLastAcquireTime());
    }
  }

  return true;
}

ezUInt32 ezResourceManager::ReloadResourcesOfType(const ezRTTI* pType, bool bForce)
{
  EZ_LOCK(s_ResourceMutex);
  EZ_LOG_BLOCK("ezResourceManager::ReloadResourcesOfType", pType->GetTypeName());

  ezUInt32 count = 0;

  LoadedResources& lr = s_LoadedResources[pType];

  for (auto it = lr.m_Resources.GetIterator(); it.IsValid(); ++it)
  {
    if (ReloadResource(it.Value(), bForce))
      ++count;
  }

  return count;
}

ezUInt32 ezResourceManager::ReloadAllResources(bool bForce)
{
  EZ_LOCK(s_ResourceMutex);
  EZ_LOG_BLOCK("ezResourceManager::ReloadAllResources");

  ezUInt32 count = 0;

  for (auto itType = s_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
  {
    for (auto it = itType.Value().m_Resources.GetIterator(); it.IsValid(); ++it)
    {
      if (ReloadResource(it.Value(), bForce))
        ++count;
    }
  }

  if (count > 0)
  {
    ezResourceManagerEvent e;
    e.m_Type = ezResourceManagerEvent::Type::ReloadAllResources;

    s_ManagerEvents.Broadcast(e);
  }

  return count;
}

bool ezResourceManager::HelpResourceLoading()
{
  // the ezTaskSystem will take care of executing other tasks on this thread when we call WaitForTask
  // so this function will 'help' by 'waiting' for some other task

  for (ezInt32 i = 0; i < MaxUpdateContentTasks; ++i)
  {
    // get the 'oldest' main thread task in the queue and try to finish that first
    const ezInt32 iWorkerMainThread = (ezResourceManager::s_uiCurrentUpdateContentWorkerTask + i) % MaxUpdateContentTasks;

    if (!s_WorkerTasksUpdateContent[iWorkerMainThread].IsTaskFinished())
    {
      ezTaskSystem::WaitForTask(&s_WorkerTasksUpdateContent[iWorkerMainThread]);
      return true; // we waited for one of them, that's enough for this round
    }
  }

  if (!s_WorkerTasksDataLoad[s_uiCurrentLoadDataWorkerTask].IsTaskFinished())
  {
    ezTaskSystem::WaitForTask(&s_WorkerTasksDataLoad[s_uiCurrentLoadDataWorkerTask]);
    return true;
  }

  return false;
}


void ezResourceManager::UpdateResourceWithCustomLoader(
  const ezTypelessResourceHandle& hResource, ezUniquePtr<ezResourceTypeLoader>&& loader)
{
  EZ_LOCK(s_ResourceMutex);

  hResource.m_pResource->m_Flags.Add(ezResourceFlags::HasCustomDataLoader);
  s_CustomLoaders[hResource.m_pResource] = std::move(loader);
  // if there was already a custom loader set, but it got no action yet, it is deleted here and replaced with the newer loader

  ReloadResource(hResource.m_pResource, true);
};

void ezResourceManager::EnsureResourceLoadingState(ezResource* pResource, const ezResourceState RequestedState)
{
  // help loading until the requested resource is available
  while ((ezInt32)pResource->GetLoadingState() < (ezInt32)RequestedState &&
         (pResource->GetLoadingState() != ezResourceState::LoadedResourceMissing))
  {
    HelpResourceLoading();
  }
}
