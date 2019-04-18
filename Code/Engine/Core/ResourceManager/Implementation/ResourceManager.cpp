#include <CorePCH.h>

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

ezUInt32 ezResourceManager::s_uiForceNoFallbackAcquisition = 0;
ezHashTable<const ezRTTI*, ezResourceManager::LoadedResources> ezResourceManager::s_LoadedResources;
ezMap<const ezRTTI*, ezResourceTypeLoader*> ezResourceManager::s_ResourceTypeLoader;
ezResourceLoaderFromFile ezResourceManager::s_FileResourceLoader;
ezResourceTypeLoader* ezResourceManager::s_pDefaultResourceLoader = &s_FileResourceLoader;
ezDeque<ezResourceManager::LoadingInfo> ezResourceManager::s_RequireLoading;
bool ezResourceManager::s_bTaskRunning = false;
bool ezResourceManager::s_bShutdown = false;
bool ezResourceManager::s_bExportMode = false;
ezUInt32 ezResourceManager::s_uiNextResourceID = 0;
ezResourceManagerWorkerDataLoad ezResourceManager::s_WorkerTasksDataLoad[MaxDataLoadTasks];
ezResourceManagerWorkerUpdateContent ezResourceManager::s_WorkerTasksUpdateContent[MaxUpdateContentTasks];
ezUInt8 ezResourceManager::s_uiCurrentUpdateContentWorkerTask = 0;
ezUInt8 ezResourceManager::s_uiCurrentLoadDataWorkerTask = 0;
ezTime ezResourceManager::s_LastFrameUpdate;
ezUInt32 ezResourceManager::s_uiLastResourcePriorityUpdateIdx = 0;
ezDynamicArray<ezResource*> ezResourceManager::s_LoadedResourceOfTypeTempContainer;
ezHashTable<ezTempHashedString, const ezRTTI*> ezResourceManager::s_ResourcesToUnloadOnMainThread;
bool ezResourceManager::s_bBroadcastExistsEvent = false;
ezEvent<const ezResourceEvent&> ezResourceManager::s_ResourceEvents;
ezEvent<const ezResourceManagerEvent&> ezResourceManager::s_ManagerEvents;
ezMutex ezResourceManager::s_ResourceMutex;
ezHashTable<ezTempHashedString, ezHashedString> ezResourceManager::s_NamedResources;
ezMap<ezString, const ezRTTI*> ezResourceManager::s_AssetToResourceType;
ezMap<ezResource*, ezUniquePtr<ezResourceTypeLoader>> ezResourceManager::s_CustomLoaders;
ezMap<const ezRTTI*, ezHybridArray<ezResourceManager::DerivedTypeInfo, 4>> ezResourceManager::s_DerivedTypeInfos;
ezDynamicArray<ezResourceManager::ResourceCleanupCB> ezResourceManager::s_ResourceCleanupCallbacks;
ezMap<const ezRTTI*, ezResourcePriority> ezResourceManager::s_ResourceTypePriorities;

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
  return s_ResourceTypeLoader[pRTTI];
}

void ezResourceManager::AddResourceCleanupCallback(ResourceCleanupCB cb)
{
  if (!s_ResourceCleanupCallbacks.Contains(cb))
  {
    s_ResourceCleanupCallbacks.PushBack(cb);
  }
}

void ezResourceManager::ClearResourceCleanupCallback(ResourceCleanupCB cb)
{
  s_ResourceCleanupCallbacks.RemoveAndSwap(cb);
}

void ezResourceManager::ExecuteAllResourceCleanupCallbacks()
{
  ezDynamicArray<ResourceCleanupCB> callbacks = s_ResourceCleanupCallbacks;
  s_ResourceCleanupCallbacks.Clear();

  for (auto& cb : callbacks)
  {
    cb();
  }

  EZ_ASSERT_DEV(s_ResourceCleanupCallbacks.IsEmpty(), "During resource cleanup, new resource cleanup callbacks were registered.");
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

void ezResourceManager::ForceNoFallbackAcquisition(ezUInt32 uiNumFrames /*= 0xFFFFFFFF*/)
{
  s_uiForceNoFallbackAcquisition = ezMath::Max(s_uiForceNoFallbackAcquisition, uiNumFrames);
}

ezUInt32 ezResourceManager::FreeUnusedResources(bool bFreeAllUnused)
{
  EZ_LOCK(s_ResourceMutex);
  EZ_LOG_BLOCK("ezResourceManager::FreeUnusedResources");

  EZ_PROFILE_SCOPE("FreeUnusedResources");

  ezUInt32 uiUnloaded = 0;
  bool bUnloadedAny = false;

  do
  {
    bUnloadedAny = false;

    for (auto itType = s_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
    {
      const ezRTTI* pRtti = itType.Key();
      LoadedResources& lr = itType.Value();

      for (auto it = lr.m_Resources.GetIterator(); it.IsValid(); /* empty */)
      {
        ezResource* pReference = it.Value();

        if (pReference->m_iReferenceCount > 0)
        {
          ++it;
          continue;
        }

        const auto& CurKey = it.Key();

        EZ_ASSERT_DEBUG(pReference->m_iLockCount == 0, "Resource '{0}' has a refcount of zero, but is still in an acquired state.",
          pReference->GetResourceID());

        bUnloadedAny = true;
        ++uiUnloaded;
        pReference->CallUnloadData(ezResource::Unload::AllQualityLevels);

        EZ_ASSERT_DEBUG(pReference->GetLoadingState() <= ezResourceState::UnloadedMetaInfoAvailable,
          "Resource '{0}' should be in an unloaded state now.", pReference->GetResourceID());

        // broadcast that we are going to delete the resource
        {
          ezResourceEvent e;
          e.m_pResource = pReference;
          e.m_Type = ezResourceEvent::Type::ResourceDeleted;
          ezResourceManager::BroadcastResourceEvent(e);
        }

        // delete the resource via the RTTI provided allocator
        pReference->GetDynamicRTTI()->GetAllocator()->Deallocate(pReference);

        ++it;

        lr.m_Resources.Remove(CurKey);
      }
    }

  } while (bFreeAllUnused && bUnloadedAny);

  return uiUnloaded;
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

  for (auto itType = s_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
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

  s_LastFrameUpdate = ezTime::Now();

  if (s_bBroadcastExistsEvent)
  {
    EZ_LOCK(s_ResourceMutex);

    s_bBroadcastExistsEvent = false;

    for (auto itType = s_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
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

    for (auto it = s_ResourcesToUnloadOnMainThread.GetIterator(); it.IsValid(); it.Next())
    {
      // Identify the container of loaded resource for the type of resource we want to unload.
      ezResourceManager::LoadedResources loadedResourcesForType;
      if (s_LoadedResources.TryGetValue(it.Value(), loadedResourcesForType) == false)
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

      EZ_ASSERT_DEV(resourceToUnload->GetLoadingState() <= ezResourceState::UnloadedMetaInfoAvailable,
        "Resource '{0}' should be in an unloaded state now.", resourceToUnload->GetResourceID());
    }

    s_ResourcesToUnloadOnMainThread.Clear();
  }
}

void ezResourceManager::BroadcastExistsEvent()
{
  s_bBroadcastExistsEvent = true;
}

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
  s_bTaskRunning = false;
  s_bShutdown = false;

  ezPlugin::s_PluginEvents.AddEventHandler(PluginEventHandler);
}

void ezResourceManager::EngineAboutToShutdown()
{
  {
    EZ_LOCK(s_ResourceMutex);
    s_RequireLoading.Clear();
    s_bTaskRunning = true;
    s_bShutdown = true;
  }

  for (int i = 0; i < ezResourceManager::MaxDataLoadTasks; ++i)
  {
    ezTaskSystem::CancelTask(&s_WorkerTasksDataLoad[i]);
  }

  for (int i = 0; i < ezResourceManager::MaxUpdateContentTasks; ++i)
  {
    ezTaskSystem::CancelTask(&s_WorkerTasksUpdateContent[i]);
  }
}

void ezResourceManager::OnEngineShutdown()
{
  ezResourceManagerEvent e;
  e.m_Type = ezResourceManagerEvent::Type::ManagerShuttingDown;

  // in case of a crash inside the event broadcast or ExecuteAllResourceCleanupCallbacks():
  // you might have a resource type added through a dynamic plugin that has already been unloaded,
  // but the event handler is still referenced
  // to fix this, call ezResource::CleanupDynamicPluginReferences() on that resource type during engine shutdown (see ezStartup)
  s_ManagerEvents.Broadcast(e);

  ExecuteAllResourceCleanupCallbacks();

  EngineAboutToShutdown();

  // unload all resources until there are no more that can be unloaded
  FreeUnusedResources(true);
}

void ezResourceManager::OnCoreShutdown()
{
  OnEngineShutdown();

  EZ_LOG_BLOCK("Referenced Resources");

  for (auto itType = s_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
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
      }
    }
  }

  ezPlugin::s_PluginEvents.RemoveEventHandler(PluginEventHandler);
}

ezResource* ezResourceManager::GetResource(const ezRTTI* pRtti, const char* szResourceID, bool bIsReloadable)
{
  if (ezStringUtils::IsNullOrEmpty(szResourceID))
    return nullptr;

  EZ_LOCK(s_ResourceMutex);

  // redirect requested type to override type, if available
  pRtti = FindResourceTypeOverride(pRtti, szResourceID);

  EZ_ASSERT_DEBUG(pRtti != nullptr, "There is no RTTI information available for the given resource type '{0}'", EZ_STRINGIZE(ResourceType));
  EZ_ASSERT_DEBUG(pRtti->GetAllocator() != nullptr && pRtti->GetAllocator()->CanAllocate(),
    "There is no RTTI allocator available for the given resource type '{0}'", EZ_STRINGIZE(ResourceType));

  ezResource* pResource = nullptr;
  ezTempHashedString sHashedResourceID(szResourceID);

  ezHashedString* redirection;
  if (s_NamedResources.TryGetValue(sHashedResourceID, redirection))
  {
    sHashedResourceID = *redirection;
    szResourceID = redirection->GetData();
  }

  LoadedResources& lr = s_LoadedResources[pRtti];

  if (lr.m_Resources.TryGetValue(sHashedResourceID, pResource))
    return pResource;

  ezResource* pNewResource = pRtti->GetAllocator()->Allocate<ezResource>();
  pNewResource->m_Priority = s_ResourceTypePriorities.GetValueOrDefault(pRtti, ezResourcePriority::Medium);
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
    auto& info = s_DerivedTypeInfos[pParentType].ExpandAndGetRef();
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
    auto it = s_DerivedTypeInfos.Find(pParentType);
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
  auto it = s_DerivedTypeInfos.Find(pRtti);

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
        it = s_DerivedTypeInfos.Find(pRtti);
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
  resourceID.Format("{}-{}", prefix, s_uiNextResourceID++);
  return resourceID;
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

  if (pResource->GetBaseResourceFlags().IsSet(ezResourceFlags::IsPreloading))
  {
    LoadingInfo li;
    li.m_pResource = pResource;

    if (!s_RequireLoading.RemoveAndCopy(li))
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

void ezResourceManager::EnableExportMode(bool enable)
{
  s_bExportMode = enable;
}

EZ_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_ResourceManager);
