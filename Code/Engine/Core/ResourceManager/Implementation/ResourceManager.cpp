#include <CorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceManagerState.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Profiling/Profiling.h>

/// \todo Do not unload resources while they are acquired
/// \todo Resource Type Memory Thresholds
/// \todo Preload does not load all quality levels

/// Infos to Display:
///   Ref Count (max)
///   Fallback: Type / Instance
///   Loading Time

/// Resource Flags:
/// Category / Group (Texture Sets)

/// Resource Loader
///   Requires No File Access -> on non-File Thread

ezUniquePtr<ezResourceManagerState> ezResourceManager::s_State;
ezMutex ezResourceManager::s_ResourceMutex;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, ResourceManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezResourceManager::OnCoreStartup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezResourceManager::OnCoreShutdown();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezResourceManager::OnEngineShutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on


ezResourceTypeLoader* ezResourceManager::GetResourceTypeLoader(const ezRTTI* pRTTI)
{
  return s_State->s_ResourceTypeLoader[pRTTI];
}

ezMap<const ezRTTI*, ezResourceTypeLoader*>& ezResourceManager::GetResourceTypeLoaders()
{
  return s_State->s_ResourceTypeLoader;
}

void ezResourceManager::AddResourceCleanupCallback(ResourceCleanupCB cb)
{
  EZ_ASSERT_DEV(cb.IsComparable(), "Delegates with captures are not allowed");

  for (ezUInt32 i = 0; i < s_State->s_ResourceCleanupCallbacks.GetCount(); ++i)
  {
    if (s_State->s_ResourceCleanupCallbacks[i].IsEqualIfComparable(cb))
      return;
  }

  s_State->s_ResourceCleanupCallbacks.PushBack(cb);
}

void ezResourceManager::ClearResourceCleanupCallback(ResourceCleanupCB cb)
{
  for (ezUInt32 i = 0; i < s_State->s_ResourceCleanupCallbacks.GetCount(); ++i)
  {
    if (s_State->s_ResourceCleanupCallbacks[i].IsEqualIfComparable(cb))
    {
      s_State->s_ResourceCleanupCallbacks.RemoveAtAndSwap(i);
      return;
    }
  }
}

void ezResourceManager::ExecuteAllResourceCleanupCallbacks()
{
  if (s_State == nullptr)
  {
    // In case resource manager wasn't initialized, nothing to do
    return;
  }

  ezDynamicArray<ResourceCleanupCB> callbacks = s_State->s_ResourceCleanupCallbacks;
  s_State->s_ResourceCleanupCallbacks.Clear();

  for (auto& cb : callbacks)
  {
    cb();
  }

  EZ_ASSERT_DEV(s_State->s_ResourceCleanupCallbacks.IsEmpty(), "During resource cleanup, new resource cleanup callbacks were registered.");
}

ezMap<const ezRTTI*, ezResourcePriority>& ezResourceManager::GetResourceTypePriorities()
{
  return s_State->s_ResourceTypePriorities;
}

void ezResourceManager::BroadcastResourceEvent(const ezResourceEvent& e)
{
  EZ_LOCK(s_ResourceMutex);

  // broadcast it through the resource to everyone directly interested in that specific resource
  e.m_pResource->m_ResourceEvents.Broadcast(e);

  // and then broadcast it to everyone else through the general event
  s_State->s_ResourceEvents.Broadcast(e);
}

void ezResourceManager::RegisterResourceForAssetType(const char* szAssetTypeName, const ezRTTI* pResourceType)
{
  ezStringBuilder s = szAssetTypeName;
  s.ToLower();

  s_State->s_AssetToResourceType[s] = pResourceType;
}

const ezRTTI* ezResourceManager::FindResourceForAssetType(const char* szAssetTypeName)
{
  ezStringBuilder s = szAssetTypeName;
  s.ToLower();

  return s_State->s_AssetToResourceType.GetValueOrDefault(s, nullptr);
}

void ezResourceManager::ForceNoFallbackAcquisition(ezUInt32 uiNumFrames /*= 0xFFFFFFFF*/)
{
  s_State->s_uiForceNoFallbackAcquisition = ezMath::Max(s_State->s_uiForceNoFallbackAcquisition, uiNumFrames);
}

ezUInt32 ezResourceManager::FreeAllUnusedResources()
{
  EZ_LOG_BLOCK("ezResourceManager::FreeAllUnusedResources");

  EZ_PROFILE_SCOPE("FreeAllUnusedResources");

  if (s_State == nullptr)
  {
    // In case resource manager wasn't initialized, no resources to unload
    return 0;
  }

  const bool bFreeAllUnused = true;

  ezUInt32 uiUnloaded = 0;
  bool bUnloadedAny = false;
  bool bAnyFailed = false;

  do
  {
    {
      EZ_LOCK(s_ResourceMutex);

      bUnloadedAny = false;

      for (auto itType = s_State->s_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
      {
        const ezRTTI* pRtti = itType.Key();
        LoadedResources& lr = itType.Value();

        for (auto it = lr.m_Resources.GetIterator(); it.IsValid(); /* empty */)
        {
          ezResource* pReference = it.Value();

          if (pReference->m_iReferenceCount == 0)
          {
            bUnloadedAny = true; // make sure to try again, even if DeallocateResource() fails; need to release our lock for that to prevent dead-locks

            if (DeallocateResource(pReference).Succeeded())
            {
              ++uiUnloaded;

              it = lr.m_Resources.Remove(it);
              continue;
            }
            else
            {
              bAnyFailed = true;
            }
          }

          ++it;
        }
      }
    }

    if (bAnyFailed)
    {
      // When this happens, it is possible that the resource that failed to be deleted
      // is dependent on a task that needs to be executed on THIS thread (main thread).
      // Therefore, help executing some tasks here, to unblock the task system.

      bAnyFailed = false;

      ezInt32 iHelpExecTasksRounds = 1;
      ezTaskSystem::WaitForCondition([&iHelpExecTasksRounds]() { return iHelpExecTasksRounds-- <= 0; });
    }

  } while (bFreeAllUnused && bUnloadedAny);

  return uiUnloaded;
}

ezUInt32 ezResourceManager::FreeUnusedResources(ezTime timeout, ezTime lastAcquireThreshold)
{
  if (timeout.IsZeroOrNegative())
    return 0;

  EZ_LOCK(s_ResourceMutex);
  EZ_LOG_BLOCK("ezResourceManager::FreeUnusedResources");
  EZ_PROFILE_SCOPE("FreeUnusedResources");

  auto itResourceType = s_State->s_LoadedResources.Find(s_State->s_pFreeUnusedLastType);
  if (!itResourceType.IsValid())
  {
    itResourceType = s_State->s_LoadedResources.GetIterator();
  }

  if (!itResourceType.IsValid())
    return 0;

  auto itResourceID = itResourceType.Value().m_Resources.Find(s_State->s_FreeUnusedLastResourceID);
  if (!itResourceID.IsValid())
  {
    itResourceID = itResourceType.Value().m_Resources.GetIterator();
  }

  const ezTime tStart = ezTime::Now();

  ezUInt32 uiDeallocatedCount = 0;

  ezStringBuilder sResourceName;

  const ezRTTI* pLastTypeCheck = nullptr;

  // stop once we wasted enough time
  while (ezTime::Now() - tStart < timeout)
  {
    if (!itResourceID.IsValid())
    {
      // reached the end of this resource type
      // advance to the next resource type
      ++itResourceType;

      if (!itResourceType.IsValid())
      {
        // if we reached the end, reset everything and stop

        s_State->s_pFreeUnusedLastType = nullptr;
        s_State->s_FreeUnusedLastResourceID = ezTempHashedString();
        return uiDeallocatedCount;
      }


      // reset resource ID to the beginning of this type and start over
      itResourceID = itResourceType.Value().m_Resources.GetIterator();
      continue;
    }

    s_State->s_pFreeUnusedLastType = itResourceType.Key();
    s_State->s_FreeUnusedLastResourceID = itResourceID.Key();

    if (pLastTypeCheck != itResourceType.Key())
    {
      pLastTypeCheck = itResourceType.Key();

      if (GetResourceTypeInfo(pLastTypeCheck).m_bIncrementalUnload == false)
      {
        itResourceID = itResourceType.Value().m_Resources.GetEndIterator();
        continue;
      }
    }

    ezResource* pResource = itResourceID.Value();

    if ((pResource->GetReferenceCount() == 0) && (tStart - pResource->GetLastAcquireTime() > lastAcquireThreshold))
    {
      sResourceName = pResource->GetResourceID();

      if (DeallocateResource(pResource).Succeeded())
      {
        ezLog::Debug("Freed '{}'", sResourceName);

        ++uiDeallocatedCount;
        itResourceID = itResourceType.Value().m_Resources.Remove(itResourceID);
        continue;
      }
    }

    ++itResourceID;
  }

  return uiDeallocatedCount;
}

void ezResourceManager::SetAutoFreeUnused(ezTime timeout, ezTime lastAcquireThreshold)
{
  s_State->m_AutoFreeUnusedTimeout = timeout;
  s_State->m_AutoFreeUnusedThreshold = lastAcquireThreshold;
}

void ezResourceManager::AllowResourceTypeAcquireDuringUpdateContent(const ezRTTI* pTypeBeingUpdated, const ezRTTI* pTypeItWantsToAcquire)
{
  auto& info = s_State->m_TypeInfo[pTypeBeingUpdated];

  EZ_ASSERT_DEV(info.m_bAllowNestedAcquireCached == false, "AllowResourceTypeAcquireDuringUpdateContent for type '{}' must be called before the resource info has been requested.", pTypeBeingUpdated->GetTypeName());

  if (info.m_NestedTypes.IndexOf(pTypeItWantsToAcquire) == ezInvalidIndex)
  {
    info.m_NestedTypes.PushBack(pTypeItWantsToAcquire);
  }
}

bool ezResourceManager::IsResourceTypeAcquireDuringUpdateContentAllowed(const ezRTTI* pTypeBeingUpdated, const ezRTTI* pTypeItWantsToAcquire)
{
  auto& info = s_State->m_TypeInfo[pTypeBeingUpdated];

  if (!info.m_bAllowNestedAcquireCached)
  {
    info.m_bAllowNestedAcquireCached = true;

    ezSet<const ezRTTI*> visited;
    ezSet<const ezRTTI*> todo;
    ezSet<const ezRTTI*> deps;

    for (const ezRTTI* pRtti : info.m_NestedTypes)
    {
      todo.Insert(pRtti);
    }

    while (!todo.IsEmpty())
    {
      auto it = todo.GetIterator();
      const ezRTTI* pRtti = it.Key();
      todo.Remove(it);

      if (visited.Contains(pRtti))
        continue;

      visited.Insert(pRtti);
      deps.Insert(pRtti);

      for (const ezRTTI* pNestedRtti : s_State->m_TypeInfo[pRtti].m_NestedTypes)
      {
        if (!visited.Contains(pNestedRtti))
          todo.Insert(pNestedRtti);
      }
    }

    info.m_NestedTypes.Clear();
    for (const ezRTTI* pRtti : deps)
    {
      info.m_NestedTypes.PushBack(pRtti);
    }
    info.m_NestedTypes.Sort();
  }

  return info.m_NestedTypes.IndexOf(pTypeItWantsToAcquire) != ezInvalidIndex;
}

ezResult ezResourceManager::DeallocateResource(ezResource* pResource)
{
  EZ_ASSERT_DEBUG(pResource->m_iLockCount == 0, "Resource '{0}' has a refcount of zero, but is still in an acquired state.", pResource->GetResourceID());

  if (RemoveFromLoadingQueue(pResource).Failed())
  {
    // cannot deallocate resources that are currently queued for loading,
    // especially when they are already picked up by a task
    return EZ_FAILURE;
  }

  pResource->CallUnloadData(ezResource::Unload::AllQualityLevels);

  EZ_ASSERT_DEBUG(pResource->GetLoadingState() <= ezResourceState::LoadedResourceMissing,
    "Resource '{0}' should be in an unloaded state now.", pResource->GetResourceID());

  // broadcast that we are going to delete the resource
  {
    ezResourceEvent e;
    e.m_pResource = pResource;
    e.m_Type = ezResourceEvent::Type::ResourceDeleted;
    ezResourceManager::BroadcastResourceEvent(e);
  }

  // delete the resource via the RTTI provided allocator
  pResource->GetDynamicRTTI()->GetAllocator()->Deallocate(pResource);

  return EZ_SUCCESS;
}

// To allow triggering this event without a link dependency
// Used by Fileserve, to trigger this event, even though Fileserve should not have a link dependency on Core
EZ_ON_GLOBAL_EVENT(ezResourceManager_ReloadAllResources)
{
  ezResourceManager::ReloadAllResources(false);
}
void ezResourceManager::ResetAllResources()
{
  EZ_LOCK(s_ResourceMutex);
  EZ_LOG_BLOCK("ezResourceManager::ReloadAllResources");

  for (auto itType = s_State->s_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
  {
    for (auto it = itType.Value().m_Resources.GetIterator(); it.IsValid(); ++it)
    {
      ezResource* pResource = it.Value();
      pResource->ResetResource();
    }
  }
}

void ezResourceManager::PerFrameUpdate()
{
  EZ_PROFILE_SCOPE("ezResourceManagerUpdate");

  s_State->s_LastFrameUpdate = ezTime::Now();

  if (s_State->s_bBroadcastExistsEvent)
  {
    EZ_LOCK(s_ResourceMutex);

    s_State->s_bBroadcastExistsEvent = false;

    for (auto itType = s_State->s_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
    {
      for (auto it = itType.Value().m_Resources.GetIterator(); it.IsValid(); ++it)
      {
        ezResourceEvent e;
        e.m_Type = ezResourceEvent::Type::ResourceExists;
        e.m_pResource = it.Value();

        ezResourceManager::BroadcastResourceEvent(e);
      }
    }
  }

  {
    EZ_LOCK(s_ResourceMutex);

    for (auto it = s_State->s_ResourcesToUnloadOnMainThread.GetIterator(); it.IsValid(); it.Next())
    {
      // Identify the container of loaded resource for the type of resource we want to unload.
      LoadedResources loadedResourcesForType;
      if (s_State->s_LoadedResources.TryGetValue(it.Value(), loadedResourcesForType) == false)
      {
        continue;
      }

      // See, if the resource we want to unload still exists.
      ezResource* resourceToUnload = nullptr;

      if (loadedResourcesForType.m_Resources.TryGetValue(it.Key(), resourceToUnload) == false)
      {
        continue;
      }

      EZ_ASSERT_DEV(resourceToUnload != nullptr, "Found a resource above, should not be nullptr.");

      // If the resource was still loaded, we are going to unload it now.
      resourceToUnload->CallUnloadData(ezResource::Unload::AllQualityLevels);

      EZ_ASSERT_DEV(resourceToUnload->GetLoadingState() <= ezResourceState::LoadedResourceMissing,
        "Resource '{0}' should be in an unloaded state now.", resourceToUnload->GetResourceID());
    }

    s_State->s_ResourcesToUnloadOnMainThread.Clear();
  }

  if (s_State->m_AutoFreeUnusedTimeout.IsPositive())
  {
    FreeUnusedResources(s_State->m_AutoFreeUnusedTimeout, s_State->m_AutoFreeUnusedThreshold);
  }
}

const ezEvent<const ezResourceEvent&>& ezResourceManager::GetResourceEvents()
{
  return s_State->s_ResourceEvents;
}

const ezEvent<const ezResourceManagerEvent&>& ezResourceManager::GetManagerEvents()
{
  return s_State->s_ManagerEvents;
}

void ezResourceManager::BroadcastExistsEvent()
{
  s_State->s_bBroadcastExistsEvent = true;
}

void ezResourceManager::PluginEventHandler(const ezPluginEvent& e)
{
  switch (e.m_EventType)
  {
    case ezPluginEvent::AfterStartupShutdown:
    {
      // unload all resources until there are no more that can be unloaded
      // this is to prevent having resources allocated that came from a dynamic plugin
      FreeAllUnusedResources();
    }
    break;

    default:
      break;
  }
}

void ezResourceManager::OnCoreStartup()
{
  s_State = EZ_DEFAULT_NEW(ezResourceManagerState);

  EZ_LOCK(s_ResourceMutex);
  s_State->s_bAllowLaunchDataLoadTask = true;
  s_State->s_bShutdown = false;

  ezPlugin::s_PluginEvents.AddEventHandler(PluginEventHandler);
}

void ezResourceManager::EngineAboutToShutdown()
{
  {
    EZ_LOCK(s_ResourceMutex);

    if (s_State == nullptr)
    {
      // In case resource manager wasn't initialized, nothing to do
      return;
    }

    s_State->s_bAllowLaunchDataLoadTask = false; // prevent a new one from starting
    s_State->s_bShutdown = true;
  }

  for (ezUInt32 i = 0; i < s_State->s_WorkerTasksDataLoad.GetCount(); ++i)
  {
    ezTaskSystem::CancelTask(s_State->s_WorkerTasksDataLoad[i].m_pTask.Borrow());
  }

  for (ezUInt32 i = 0; i < s_State->s_WorkerTasksUpdateContent.GetCount(); ++i)
  {
    ezTaskSystem::CancelTask(s_State->s_WorkerTasksUpdateContent[i].m_pTask.Borrow());
  }

  {
    EZ_LOCK(s_ResourceMutex);

    for (auto entry : s_State->s_LoadingQueue)
    {
      entry.m_pResource->m_Flags.Remove(ezResourceFlags::IsQueuedForLoading);
    }

    s_State->s_LoadingQueue.Clear();

    // Since we just canceled all loading tasks above and cleared the loading queue,
    // some resources may still be flagged as 'loading', but can never get loaded.
    // That can deadlock the 'FreeAllUnused' function, because it won't delete 'loading' resources.
    // Therefore we need to make sure no resource has the IsQueuedForLoading flag set anymore.
    for (auto itTypes : s_State->s_LoadedResources)
    {
      for (auto itRes : itTypes.Value().m_Resources)
      {
        ezResource* pRes = itRes.Value();

        if (pRes->GetBaseResourceFlags().IsSet(ezResourceFlags::IsQueuedForLoading))
        {
          pRes->m_Flags.Remove(ezResourceFlags::IsQueuedForLoading);
        }
      }
    }
  }
}

bool ezResourceManager::IsAnyLoadingInProgress()
{
  EZ_LOCK(s_ResourceMutex);

  if (s_State->s_LoadingQueue.GetCount() > 0)
  {
    return true;
  }

  for (ezUInt32 i = 0; i < s_State->s_WorkerTasksDataLoad.GetCount(); ++i)
  {
    if (!s_State->s_WorkerTasksDataLoad[i].m_pTask->IsTaskFinished())
    {
      return true;
    }
  }

  for (ezUInt32 i = 0; i < s_State->s_WorkerTasksUpdateContent.GetCount(); ++i)
  {
    if (!s_State->s_WorkerTasksUpdateContent[i].m_pTask->IsTaskFinished())
    {
      return true;
    }
  }
  return false;
}

void ezResourceManager::OnEngineShutdown()
{
  ezResourceManagerEvent e;
  e.m_Type = ezResourceManagerEvent::Type::ManagerShuttingDown;

  // in case of a crash inside the event broadcast or ExecuteAllResourceCleanupCallbacks():
  // you might have a resource type added through a dynamic plugin that has already been unloaded,
  // but the event handler is still referenced
  // to fix this, call ezResource::CleanupDynamicPluginReferences() on that resource type during engine shutdown (see ezStartup)
  s_State->s_ManagerEvents.Broadcast(e);

  ExecuteAllResourceCleanupCallbacks();

  EngineAboutToShutdown();

  // unload all resources until there are no more that can be unloaded
  FreeAllUnusedResources();
}

void ezResourceManager::OnCoreShutdown()
{
  OnEngineShutdown();

  EZ_LOG_BLOCK("Referenced Resources");

  for (auto itType = s_State->s_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
  {
    const ezRTTI* pRtti = itType.Key();
    LoadedResources& lr = itType.Value();

    if (!lr.m_Resources.IsEmpty())
    {
      EZ_LOG_BLOCK("Type", pRtti->GetTypeName());

      ezLog::Error("{0} resource of type '{1}' are still referenced.", lr.m_Resources.GetCount(), pRtti->GetTypeName());

      for (auto it = lr.m_Resources.GetIterator(); it.IsValid(); ++it)
      {
        ezResource* pReference = it.Value();

        ezLog::Info("RC = {0}, ID = '{1}'", pReference->GetReferenceCount(), pReference->GetResourceID());

#if EZ_ENABLED(EZ_RESOURCEHANDLE_STACK_TRACES)
        pReference->PrintHandleStackTraces();
#endif
      }
    }
  }

  ezPlugin::s_PluginEvents.RemoveEventHandler(PluginEventHandler);

  s_State.Clear();
}

ezResource* ezResourceManager::GetResource(const ezRTTI* pRtti, const char* szResourceID, bool bIsReloadable)
{
  if (ezStringUtils::IsNullOrEmpty(szResourceID))
    return nullptr;

  EZ_ASSERT_DEV(s_ResourceMutex.IsLocked(), "Calling code must lock the mutex until the resource pointer is stored in a handle");

  // redirect requested type to override type, if available
  pRtti = FindResourceTypeOverride(pRtti, szResourceID);

  EZ_ASSERT_DEBUG(pRtti != nullptr, "There is no RTTI information available for the given resource type '{0}'", EZ_STRINGIZE(ResourceType));
  EZ_ASSERT_DEBUG(pRtti->GetAllocator() != nullptr && pRtti->GetAllocator()->CanAllocate(),
    "There is no RTTI allocator available for the given resource type '{0}'", EZ_STRINGIZE(ResourceType));

  ezResource* pResource = nullptr;
  ezTempHashedString sHashedResourceID(szResourceID);

  ezHashedString* redirection;
  if (s_State->s_NamedResources.TryGetValue(sHashedResourceID, redirection))
  {
    sHashedResourceID = *redirection;
    szResourceID = redirection->GetData();
  }

  LoadedResources& lr = s_State->s_LoadedResources[pRtti];

  if (lr.m_Resources.TryGetValue(sHashedResourceID, pResource))
    return pResource;

  ezResource* pNewResource = pRtti->GetAllocator()->Allocate<ezResource>();
  pNewResource->m_Priority = s_State->s_ResourceTypePriorities.GetValueOrDefault(pRtti, ezResourcePriority::Medium);
  pNewResource->SetUniqueID(szResourceID, bIsReloadable);
  pNewResource->m_Flags.AddOrRemove(ezResourceFlags::ResourceHasTypeFallback, pNewResource->HasResourceTypeLoadingFallback());

  lr.m_Resources.Insert(sHashedResourceID, pNewResource);

  return pNewResource;
}

void ezResourceManager::RegisterResourceOverrideType(
  const ezRTTI* pDerivedTypeToUse, ezDelegate<bool(const ezStringBuilder&)> OverrideDecider)
{
  const ezRTTI* pParentType = pDerivedTypeToUse->GetParentType();
  while (pParentType != nullptr && pParentType != ezGetStaticRTTI<ezResource>())
  {
    auto& info = s_State->s_DerivedTypeInfos[pParentType].ExpandAndGetRef();
    info.m_pDerivedType = pDerivedTypeToUse;
    info.m_Decider = OverrideDecider;

    pParentType = pParentType->GetParentType();
  }
}

void ezResourceManager::UnregisterResourceOverrideType(const ezRTTI* pDerivedTypeToUse)
{
  const ezRTTI* pParentType = pDerivedTypeToUse->GetParentType();
  while (pParentType != nullptr && pParentType != ezGetStaticRTTI<ezResource>())
  {
    auto it = s_State->s_DerivedTypeInfos.Find(pParentType);
    pParentType = pParentType->GetParentType();

    if (!it.IsValid())
      break;

    auto& infos = it.Value();

    for (ezUInt32 i = infos.GetCount(); i > 0; --i)
    {
      if (infos[i - 1].m_pDerivedType == pDerivedTypeToUse)
        infos.RemoveAtAndSwap(i - 1);
    }
  }
}

const ezRTTI* ezResourceManager::FindResourceTypeOverride(const ezRTTI* pRtti, const char* szResourceID)
{
  auto it = s_State->s_DerivedTypeInfos.Find(pRtti);

  if (!it.IsValid())
    return pRtti;

  ezStringBuilder sRedirectedPath;
  ezFileSystem::ResolveAssetRedirection(szResourceID, sRedirectedPath);

  while (it.IsValid())
  {
    for (const auto& info : it.Value())
    {
      if (info.m_Decider(sRedirectedPath))
      {
        pRtti = info.m_pDerivedType;
        it = s_State->s_DerivedTypeInfos.Find(pRtti);
        continue;
      }
    }

    break;
  }

  return pRtti;
}

ezString ezResourceManager::GenerateUniqueResourceID(const char* prefix)
{
  ezStringBuilder resourceID;
  resourceID.Format("{}-{}", prefix, s_State->s_uiNextResourceID++);
  return resourceID;
}

ezTypelessResourceHandle ezResourceManager::GetExistingResourceByType(const ezRTTI* pResourceType, const char* szResourceID)
{
  ezResource* pResource = nullptr;

  const ezTempHashedString sResourceHash(szResourceID);

  EZ_LOCK(s_ResourceMutex);

  const ezRTTI* pRtti = FindResourceTypeOverride(pResourceType, szResourceID);

  if (s_State->s_LoadedResources[pRtti].m_Resources.TryGetValue(sResourceHash, pResource))
    return ezTypelessResourceHandle(pResource);

  return ezTypelessResourceHandle();
}

void ezResourceManager::ForceLoadResourceNow(const ezTypelessResourceHandle& hResource)
{
  EZ_ASSERT_DEV(hResource.IsValid(), "Cannot access an invalid resource");

  ezResource* pResource = hResource.m_pResource;

  if (pResource->GetLoadingState() != ezResourceState::LoadedResourceMissing && pResource->GetLoadingState() != ezResourceState::Loaded)
  {
    InternalPreloadResource(pResource, true);

    EnsureResourceLoadingState(hResource.m_pResource, ezResourceState::Loaded);
  }
}

void ezResourceManager::RegisterNamedResource(const char* szLookupName, const char* szRedirectionResource)
{
  EZ_LOCK(s_ResourceMutex);

  ezTempHashedString lookup(szLookupName);

  ezHashedString redirection;
  redirection.Assign(szRedirectionResource);

  s_State->s_NamedResources[lookup] = redirection;
}

void ezResourceManager::UnregisterNamedResource(const char* szLookupName)
{
  EZ_LOCK(s_ResourceMutex);

  ezTempHashedString hash(szLookupName);
  s_State->s_NamedResources.Remove(hash);
}

void ezResourceManager::SetResourceLowResData(const ezTypelessResourceHandle& hResource, ezStreamReader* pStream)
{
  ezResource* pResource = hResource.m_pResource;

  if (pResource->GetBaseResourceFlags().IsSet(ezResourceFlags::HasLowResData))
    return;

  if (!pResource->GetBaseResourceFlags().IsSet(ezResourceFlags::IsReloadable))
    return;

  EZ_LOCK(s_ResourceMutex);

  // set this, even if we don't end up using the data (because some thread is already loading the full thing)
  pResource->m_Flags.Add(ezResourceFlags::HasLowResData);

  if (IsQueuedForLoading(pResource))
  {
    // if we cannot find it in the queue anymore, some thread already started loading it
    // in this case, do not try to modify it
    if (RemoveFromLoadingQueue(pResource).Failed())
      return;
  }

  pResource->CallUpdateContent(pStream);

  EZ_ASSERT_DEV(pResource->GetLoadingState() != ezResourceState::Unloaded, "The resource should have changed its loading state.");

  // Update Memory Usage
  {
    ezResource::MemoryUsage MemUsage;
    MemUsage.m_uiMemoryCPU = 0xFFFFFFFF;
    MemUsage.m_uiMemoryGPU = 0xFFFFFFFF;
    pResource->UpdateMemoryUsage(MemUsage);

    EZ_ASSERT_DEV(
      MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its CPU memory usage", pResource->GetResourceID());
    EZ_ASSERT_DEV(
      MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its GPU memory usage", pResource->GetResourceID());

    pResource->m_MemoryUsage = MemUsage;
  }
}

ezResourceTypeLoader* ezResourceManager::GetDefaultResourceLoader()
{
  return s_State->s_pDefaultResourceLoader;
}

void ezResourceManager::EnableExportMode(bool enable)
{
  EZ_ASSERT_DEV(s_State != nullptr, "ezStartup::StartupCoreSystems() must be called before using the ezResourceManager.");

  s_State->s_bExportMode = enable;
}

bool ezResourceManager::IsExportModeEnabled()
{
  EZ_ASSERT_DEV(s_State != nullptr, "ezStartup::StartupCoreSystems() must be called before using the ezResourceManager.");

  return s_State->s_bExportMode;
}

void ezResourceManager::RestoreResource(const ezTypelessResourceHandle& hResource)
{
  EZ_ASSERT_DEV(hResource.IsValid(), "Cannot access an invalid resource");

  ezResource* pResource = hResource.m_pResource;
  pResource->m_Flags.Remove(ezResourceFlags::PreventFileReload);

  ReloadResource(pResource, true);
}

ezUInt32 ezResourceManager::GetForceNoFallbackAcquisition()
{
  return s_State->s_uiForceNoFallbackAcquisition;
}

ezTime ezResourceManager::GetLastFrameUpdate()
{
  return s_State->s_LastFrameUpdate;
}

ezHashTable<const ezRTTI*, ezResourceManager::LoadedResources>& ezResourceManager::GetLoadedResources()
{
  return s_State->s_LoadedResources;
}

ezDynamicArray<ezResource*>& ezResourceManager::GetLoadedResourceOfTypeTempContainer()
{
  return s_State->s_LoadedResourceOfTypeTempContainer;
}

void ezResourceManager::SetDefaultResourceLoader(ezResourceTypeLoader* pDefaultLoader)
{
  EZ_LOCK(s_ResourceMutex);

  s_State->s_pDefaultResourceLoader = pDefaultLoader;
}

ezResourceManager::ResourceTypeInfo& ezResourceManager::GetResourceTypeInfo(const ezRTTI* pRtti)
{
  return s_State->m_TypeInfo[pRtti];
}

EZ_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_ResourceManager);
