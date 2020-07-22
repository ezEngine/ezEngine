#include <CorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceManagerState.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Profiling/Profiling.h>

ezTypelessResourceHandle ezResourceManager::LoadResourceByType(const ezRTTI* pResourceType, const char* szResourceID)
{
  // the mutex here is necessary to prevent a race between resource unloading and storing the pointer in the handle
  EZ_LOCK(s_ResourceMutex);
  return ezTypelessResourceHandle(GetResource(pResourceType, szResourceID, true));
}

void ezResourceManager::InternalPreloadResource(ezResource* pResource, bool bHighestPriority)
{
  if (s_State->s_bShutdown)
    return;

  EZ_PROFILE_SCOPE("InternalPreloadResource");

  EZ_LOCK(s_ResourceMutex);

  // if there is nothing else that could be loaded, just return right away
  if (pResource->GetLoadingState() == ezResourceState::Loaded && pResource->GetNumQualityLevelsLoadable() == 0)
  {
    // due to the threading this can happen for all resource types and is valid
    // EZ_ASSERT_DEV(!IsQueuedForLoading(pResource), "Invalid flag on resource type '{0}'",
    // pResource->GetDynamicRTTI()->GetTypeName());
    return;
  }

  EZ_ASSERT_DEV(!s_State->s_bExportMode, "Resources should not be loaded in export mode");

  // if we are already loading this resource, early out
  if (IsQueuedForLoading(pResource))
  {
    // however, if it now has highest priority and is still in the loading queue (so not yet started)
    // move it to the front of the queue
    if (bHighestPriority)
    {
      // if it is not in the queue anymore, it has already been started by some thread
      if (RemoveFromLoadingQueue(pResource).Succeeded())
      {
        AddToLoadingQueue(pResource, bHighestPriority);
      }
    }

    return;
  }
  else
  {
    AddToLoadingQueue(pResource, bHighestPriority);

    if (bHighestPriority && ezTaskSystem::GetCurrentThreadWorkerType() == ezWorkerThreadType::FileAccess)
    {
      ezResourceManager::s_State->s_bAllowLaunchDataLoadTask = true;
    }

    RunWorkerTask(pResource);
  }
}

void ezResourceManager::SetupWorkerTasks()
{
  if (!s_State->m_bTaskNamesInitialized)
  {
    s_State->m_bTaskNamesInitialized = true;
    ezStringBuilder s;

    {
      static const ezUInt32 InitialDataLoadTasks = 4;

      for (ezUInt32 i = 0; i < InitialDataLoadTasks; ++i)
      {
        s.Format("Resource Data Loader {0}", i);
        auto& data = s_State->s_WorkerTasksDataLoad.ExpandAndGetRef();
        data.m_pTask = EZ_DEFAULT_NEW(ezResourceManagerWorkerDataLoad);
        data.m_pTask->ConfigureTask(s, ezTaskNesting::Maybe);
      }
    }

    {
      static const ezUInt32 InitialUpdateContentTasks = 16;

      for (ezUInt32 i = 0; i < InitialUpdateContentTasks; ++i)
      {
        s.Format("Resource Content Updater {0}", i);
        auto& data = s_State->s_WorkerTasksUpdateContent.ExpandAndGetRef();
        data.m_pTask = EZ_DEFAULT_NEW(ezResourceManagerWorkerUpdateContent);
        data.m_pTask->ConfigureTask(s, ezTaskNesting::Maybe);
      }
    }
  }
}

void ezResourceManager::RunWorkerTask(ezResource* pResource)
{
  if (s_State->s_bShutdown)
    return;

  EZ_ASSERT_DEV(s_ResourceMutex.IsLocked(), "");

  SetupWorkerTasks();

  if (s_State->s_bAllowLaunchDataLoadTask && !s_State->s_LoadingQueue.IsEmpty())
  {
    s_State->s_bAllowLaunchDataLoadTask = false;

    for (ezUInt32 i = 0; i < s_State->s_WorkerTasksDataLoad.GetCount(); ++i)
    {
      if (s_State->s_WorkerTasksDataLoad[i].m_pTask->IsTaskFinished())
      {
        s_State->s_WorkerTasksDataLoad[i].m_GroupId =
          ezTaskSystem::StartSingleTask(s_State->s_WorkerTasksDataLoad[i].m_pTask, ezTaskPriority::FileAccess);
        return;
      }
    }

    // could not find any unused task -> need to create a new one
    {
      ezStringBuilder s;
      s.Format("Resource Data Loader {0}", s_State->s_WorkerTasksDataLoad.GetCount());
      auto& data = s_State->s_WorkerTasksDataLoad.ExpandAndGetRef();
      data.m_pTask = EZ_DEFAULT_NEW(ezResourceManagerWorkerDataLoad);
      data.m_pTask->ConfigureTask(s, ezTaskNesting::Maybe);
      data.m_GroupId = ezTaskSystem::StartSingleTask(data.m_pTask, ezTaskPriority::FileAccess);
    }
  }
}

void ezResourceManager::ReverseBubbleSortStep(ezDeque<LoadingInfo>& data)
{
  // Yep, it's really bubble sort!
  // This will move the entry with the smallest value to the front and move all other values closer to their correct position,
  // which is exactly what we need for the priority queue.
  // We do this once a frame, which gives us nice iterative sorting, with relatively deterministic performance characteristics.

  EZ_ASSERT_DEBUG(s_ResourceMutex.IsLocked(), "Calling code must acquire s_ResourceMutex");

  const ezUInt32 uiCount = data.GetCount();

  for (ezUInt32 i = uiCount; i > 1; --i)
  {
    const ezUInt32 idx2 = i - 1;
    const ezUInt32 idx1 = i - 2;

    if (data[idx1].m_fPriority > data[idx1].m_fPriority)
    {
      ezMath::Swap(data[idx1], data[idx2]);
    }
  }
}

void ezResourceManager::UpdateLoadingDeadlines()
{
  if (s_State->s_LoadingQueue.IsEmpty())
    return;

  EZ_ASSERT_DEBUG(s_ResourceMutex.IsLocked(), "Calling code must acquire s_ResourceMutex");

  EZ_PROFILE_SCOPE("UpdateLoadingDeadlines");

  const ezUInt32 uiCount = s_State->s_LoadingQueue.GetCount();
  s_State->s_uiLastResourcePriorityUpdateIdx = ezMath::Min(s_State->s_uiLastResourcePriorityUpdateIdx, uiCount);

  ezUInt32 uiUpdateCount = ezMath::Min(50u, uiCount - s_State->s_uiLastResourcePriorityUpdateIdx);

  if (uiUpdateCount == 0)
  {
    s_State->s_uiLastResourcePriorityUpdateIdx = 0;
    uiUpdateCount = ezMath::Min(50u, uiCount - s_State->s_uiLastResourcePriorityUpdateIdx);
  }

  if (uiUpdateCount > 0)
  {
    {
      EZ_PROFILE_SCOPE("EvalLoadingDeadlines");

      const ezTime tNow = ezTime::Now();

      for (ezUInt32 i = 0; i < uiUpdateCount; ++i)
      {
        auto& element = s_State->s_LoadingQueue[s_State->s_uiLastResourcePriorityUpdateIdx];
        element.m_fPriority = element.m_pResource->GetLoadingPriority(tNow);
        ++s_State->s_uiLastResourcePriorityUpdateIdx;
      }
    }

    {
      EZ_PROFILE_SCOPE("SortLoadingDeadlines");
      ReverseBubbleSortStep(s_State->s_LoadingQueue);
    }
  }
}

void ezResourceManager::PreloadResource(ezResource* pResource)
{
  InternalPreloadResource(pResource, false);
}

void ezResourceManager::PreloadResource(const ezTypelessResourceHandle& hResource)
{
  EZ_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

  ezResource* pResource = hResource.m_pResource;
  PreloadResource(hResource.m_pResource);
}

ezResourceState ezResourceManager::GetLoadingState(const ezTypelessResourceHandle& hResource)
{
  if (hResource.m_pResource == nullptr)
    return ezResourceState::Invalid;

  return hResource.m_pResource->GetLoadingState();
}

ezResult ezResourceManager::RemoveFromLoadingQueue(ezResource* pResource)
{
  EZ_ASSERT_DEV(s_ResourceMutex.IsLocked(), "Resource mutex must be locked");

  if (!IsQueuedForLoading(pResource))
    return EZ_SUCCESS;

  LoadingInfo li;
  li.m_pResource = pResource;

  if (s_State->s_LoadingQueue.RemoveAndSwap(li))
  {
    pResource->m_Flags.Remove(ezResourceFlags::IsQueuedForLoading);
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezResourceManager::AddToLoadingQueue(ezResource* pResource, bool bHighestPriority)
{
  EZ_ASSERT_DEV(s_ResourceMutex.IsLocked(), "Resource mutex must be locked");
  EZ_ASSERT_DEV(IsQueuedForLoading(pResource) == false, "Resource is already in the loading queue");

  pResource->m_Flags.Add(ezResourceFlags::IsQueuedForLoading);

  LoadingInfo li;
  li.m_pResource = pResource;

  if (bHighestPriority)
  {
    pResource->SetPriority(ezResourcePriority::Critical);
    li.m_fPriority = 0.0f;
    s_State->s_LoadingQueue.PushFront(li);
  }
  else
  {
    li.m_fPriority = pResource->GetLoadingPriority(s_State->s_LastFrameUpdate);
    s_State->s_LoadingQueue.PushBack(li);
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

  if (pLoader == nullptr)
    pLoader = pResource->GetDefaultResourceTypeLoader();

  if (pLoader == nullptr)
    return false;

  // no need to reload resources that are not loaded so far
  if (pResource->GetLoadingState() == ezResourceState::Unloaded)
    return false;

  bool bAllowPreloading = true;

  // if the resource is already in the loading queue we can just keep it there
  if (IsQueuedForLoading(pResource))
  {
    bAllowPreloading = false;

    LoadingInfo li;
    li.m_pResource = pResource;

    if (s_State->s_LoadingQueue.IndexOf(li) == ezInvalidIndex)
    {
      // the resource is marked as 'loading' but it is not in the queue anymore
      // that means some task is already working on loading it
      // therefore we should not touch it (especially unload it), it might end up in an inconsistent state

      ezLog::Dev(
        "Resource '{0}' is not being reloaded, because it is currently being loaded", ezArgSensitive(pResource->GetResourceID(), "ResourceID"));
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
      ezLog::Dev("Resource '{0}' is missing and will be tried to be reloaded ('{1}')", ezArgSensitive(pResource->GetResourceID(), "ResourceID"),
        ezArgSensitive(pResource->GetResourceDescription(), "ResourceDesc"));
    }
    else
    {
      ezLog::Dev("Resource '{0}' is outdated and will be reloaded ('{1}')", ezArgSensitive(pResource->GetResourceID(), "ResourceID"),
        ezArgSensitive(pResource->GetResourceDescription(), "ResourceDesc"));
    }
  }

  if (pResource->GetBaseResourceFlags().IsSet(ezResourceFlags::UpdateOnMainThread) == false || ezThreadUtils::IsMainThread())
  {
    // make sure existing data is purged
    pResource->CallUnloadData(ezResource::Unload::AllQualityLevels);

    EZ_ASSERT_DEV(pResource->GetLoadingState() <= ezResourceState::LoadedResourceMissing, "Resource '{0}' should be in an unloaded state now.",
      pResource->GetResourceID());
  }
  else
  {
    s_State->s_ResourcesToUnloadOnMainThread.Insert(ezTempHashedString(pResource->GetResourceID().GetData()), pResource->GetDynamicRTTI());
  }

  if (bAllowPreloading)
  {
    const ezTime tNow = s_State->s_LastFrameUpdate;

    // resources that have been in use recently will be put into the preload queue immediately
    // everything else will be loaded on demand
    if (pResource->GetLastAcquireTime() >= tNow - ezTime::Seconds(30.0))
    {
      PreloadResource(pResource);
    }
  }

  return true;
}

ezUInt32 ezResourceManager::ReloadResourcesOfType(const ezRTTI* pType, bool bForce)
{
  EZ_LOCK(s_ResourceMutex);
  EZ_LOG_BLOCK("ezResourceManager::ReloadResourcesOfType", pType->GetTypeName());

  ezUInt32 count = 0;

  LoadedResources& lr = s_State->s_LoadedResources[pType];

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

  for (auto itType = s_State->s_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
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

    s_State->s_ManagerEvents.Broadcast(e);
  }

  return count;
}

void ezResourceManager::UpdateResourceWithCustomLoader(const ezTypelessResourceHandle& hResource, ezUniquePtr<ezResourceTypeLoader>&& loader)
{
  EZ_LOCK(s_ResourceMutex);

  hResource.m_pResource->m_Flags.Add(ezResourceFlags::HasCustomDataLoader);
  s_State->s_CustomLoaders[hResource.m_pResource] = std::move(loader);
  // if there was already a custom loader set, but it got no action yet, it is deleted here and replaced with the newer loader

  ReloadResource(hResource.m_pResource, true);
};

void ezResourceManager::EnsureResourceLoadingState(ezResource* pResourceToLoad, const ezResourceState RequestedState)
{
  const ezRTTI* pOwnRtti = pResourceToLoad->GetDynamicRTTI();

  // help loading until the requested resource is available
  while ((ezInt32)pResourceToLoad->GetLoadingState() < (ezInt32)RequestedState &&
         (pResourceToLoad->GetLoadingState() != ezResourceState::LoadedResourceMissing))
  {
    ezTaskGroupID tgid;

    {
      EZ_LOCK(s_ResourceMutex);

      for (ezUInt32 i = 0; i < s_State->s_WorkerTasksUpdateContent.GetCount(); ++i)
      {
        const ezResource* pQueuedResource = s_State->s_WorkerTasksUpdateContent[i].m_pTask->m_pResourceToLoad;

        if (pQueuedResource != nullptr && pQueuedResource != pResourceToLoad && !s_State->s_WorkerTasksUpdateContent[i].m_pTask->IsTaskFinished())
        {
          if (!IsResourceTypeAcquireDuringUpdateContentAllowed(pQueuedResource->GetDynamicRTTI(), pOwnRtti))
          {
            tgid = s_State->s_WorkerTasksUpdateContent[i].m_GroupId;
            break;
          }
        }
      }
    }

    if (tgid.IsValid())
    {
      ezTaskSystem::WaitForGroup(tgid);
    }
    else
    {
      // do not use ezThreadUtils::YieldTimeSlice here, otherwise the thread is not tagged as 'blocked' in the TaskSystem
      ezTaskSystem::WaitForCondition([=]() -> bool {
        return (ezInt32)pResourceToLoad->GetLoadingState() >= (ezInt32)RequestedState ||
               (pResourceToLoad->GetLoadingState() == ezResourceState::LoadedResourceMissing);
      });
    }
  }
}


EZ_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_ResourceLoading);
