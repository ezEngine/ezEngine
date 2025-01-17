#include <Core/CorePCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/System/StackTracer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezResource, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResource::DoUpdate ezResource::UpdateGraphicsResource = ezResource::DoUpdate::OnAnyThread;

EZ_CORE_DLL void IncreaseResourceRefCount(ezResource* pResource, const void* pOwner)
{
#if EZ_ENABLED(EZ_RESOURCEHANDLE_STACK_TRACES)
  {
    EZ_LOCK(pResource->m_HandleStackTraceMutex);

    auto& info = pResource->m_HandleStackTraces[pOwner];

    ezArrayPtr<void*> ptr(info.m_Ptrs);

    info.m_uiNumPtrs = ezStackTracer::GetStackTrace(ptr);
  }
#else
  EZ_IGNORE_UNUSED(pOwner);
#endif

  pResource->m_iReferenceCount.Increment();
}

EZ_CORE_DLL void DecreaseResourceRefCount(ezResource* pResource, const void* pOwner)
{
#if EZ_ENABLED(EZ_RESOURCEHANDLE_STACK_TRACES)
  {
    EZ_LOCK(pResource->m_HandleStackTraceMutex);

    if (!pResource->m_HandleStackTraces.Remove(pOwner, nullptr))
    {
      EZ_REPORT_FAILURE("No associated stack-trace!");
    }
  }
#else
  EZ_IGNORE_UNUSED(pOwner);
#endif

  pResource->m_iReferenceCount.Decrement();
}

#if EZ_ENABLED(EZ_RESOURCEHANDLE_STACK_TRACES)
EZ_CORE_DLL void MigrateResourceRefCount(ezResource* pResource, const void* pOldOwner, const void* pNewOwner)
{
  EZ_LOCK(pResource->m_HandleStackTraceMutex);

  // allocate / resize the hash-table first to ensure the iterator stays valid
  auto& newInfo = pResource->m_HandleStackTraces[pNewOwner];

  auto it = pResource->m_HandleStackTraces.Find(pOldOwner);
  if (!it.IsValid())
  {
    EZ_REPORT_FAILURE("No associated stack-trace!");
  }
  else
  {
    newInfo = it.Value();
    pResource->m_HandleStackTraces.Remove(it);
  }
}
#endif

ezResource::~ezResource()
{
  EZ_ASSERT_DEV(!ezResourceManager::IsQueuedForLoading(this), "Cannot deallocate a resource while it is still qeued for loading");
}

ezResource::ezResource(DoUpdate ResourceUpdateThread, ezUInt8 uiQualityLevelsLoadable)
{
  if (ResourceUpdateThread == DoUpdate::OnGraphicsResourceThreads)
  {
    ResourceUpdateThread = UpdateGraphicsResource;
  }

  m_Flags.AddOrRemove(ezResourceFlags::UpdateOnMainThread, ResourceUpdateThread == DoUpdate::OnMainThread);

  m_uiQualityLevelsLoadable = uiQualityLevelsLoadable;
}

#if EZ_ENABLED(EZ_RESOURCEHANDLE_STACK_TRACES)
static void LogStackTrace(const char* szText)
{
  ezLog::Info(szText);
};
#endif

void ezResource::PrintHandleStackTraces()
{
#if EZ_ENABLED(EZ_RESOURCEHANDLE_STACK_TRACES)

  EZ_LOCK(m_HandleStackTraceMutex);

  EZ_LOG_BLOCK("Resource Handle Stack Traces");

  for (auto& it : m_HandleStackTraces)
  {
    EZ_LOG_BLOCK("Handle Trace");

    ezStackTracer::ResolveStackTrace(ezArrayPtr<void*>(it.Value().m_Ptrs, it.Value().m_uiNumPtrs), LogStackTrace);
  }

#else

  ezLog::Warning("Compile with EZ_RESOURCEHANDLE_STACK_TRACES set to EZ_ON to enable support for resource handle stack traces.");

#endif
}

void ezResource::SetResourceDescription(ezStringView sDescription)
{
  m_sResourceDescription = sDescription;
}

void ezResource::SetUniqueID(ezStringView sUniqueID, bool bIsReloadable)
{
  m_sUniqueID = sUniqueID;
  m_uiUniqueIDHash = ezHashingUtils::StringHash(sUniqueID);
  SetIsReloadable(bIsReloadable);

  ezResourceEvent e;
  e.m_pResource = this;
  e.m_Type = ezResourceEvent::Type::ResourceCreated;
  ezResourceManager::BroadcastResourceEvent(e);
}

void ezResource::CallUnloadData(Unload WhatToUnload)
{
  EZ_LOG_BLOCK("ezResource::UnloadData", GetResourceID());

  ezResourceEvent e;
  e.m_pResource = this;
  e.m_Type = ezResourceEvent::Type::ResourceContentUnloading;
  ezResourceManager::BroadcastResourceEvent(e);

  ezResourceLoadDesc ld = UnloadData(WhatToUnload);

  EZ_ASSERT_DEV(ld.m_State != ezResourceState::Invalid, "UnloadData() did not return a valid resource load state");
  EZ_ASSERT_DEV(ld.m_uiQualityLevelsDiscardable != 0xFF, "UnloadData() did not fill out m_uiQualityLevelsDiscardable correctly");
  EZ_ASSERT_DEV(ld.m_uiQualityLevelsLoadable != 0xFF, "UnloadData() did not fill out m_uiQualityLevelsLoadable correctly");

  m_LoadingState = ld.m_State;
  m_uiQualityLevelsDiscardable = ld.m_uiQualityLevelsDiscardable;
  m_uiQualityLevelsLoadable = ld.m_uiQualityLevelsLoadable;
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
thread_local const ezResource* g_pCurrentlyUpdatingContent = nullptr;

const ezResource* ezResource::GetCurrentlyUpdatingContent()
{
  return g_pCurrentlyUpdatingContent;
}
#endif

void ezResource::CallUpdateContent(ezStreamReader* Stream)
{
  EZ_PROFILE_SCOPE("CallUpdateContent");

  EZ_LOG_BLOCK("ezResource::UpdateContent", GetResourceDescription());

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  const ezResource* pPreviouslyUpdatingContent = g_pCurrentlyUpdatingContent;
  g_pCurrentlyUpdatingContent = this;
  ezResourceLoadDesc ld = UpdateContent(Stream);
  g_pCurrentlyUpdatingContent = pPreviouslyUpdatingContent;
#else
  ezResourceLoadDesc ld = UpdateContent(Stream);
#endif

  EZ_ASSERT_DEV(ld.m_State != ezResourceState::Invalid, "UpdateContent() did not return a valid resource load state");
  EZ_ASSERT_DEV(ld.m_uiQualityLevelsDiscardable != 0xFF, "UpdateContent() did not fill out m_uiQualityLevelsDiscardable correctly");
  EZ_ASSERT_DEV(ld.m_uiQualityLevelsLoadable != 0xFF, "UpdateContent() did not fill out m_uiQualityLevelsLoadable correctly");

  if (ld.m_State == ezResourceState::LoadedResourceMissing)
  {
    ReportResourceIsMissing();
  }

  IncResourceChangeCounter();

  m_uiQualityLevelsDiscardable = ld.m_uiQualityLevelsDiscardable;
  m_uiQualityLevelsLoadable = ld.m_uiQualityLevelsLoadable;
  m_LoadingState = ld.m_State;

  ezResourceEvent e;
  e.m_pResource = this;
  e.m_Type = ezResourceEvent::Type::ResourceContentUpdated;
  ezResourceManager::BroadcastResourceEvent(e);

  ezLog::Debug("Updated {0} - '{1}'", GetDynamicRTTI()->GetTypeName(), ezArgSensitive(GetResourceDescription(), "ResourceDesc"));
}

float ezResource::GetLoadingPriority(ezTime now) const
{
  if (m_Priority == ezResourcePriority::Critical)
    return 0.0f;

  // low priority values mean it gets loaded earlier
  float fPriority = static_cast<float>(m_Priority) * 10.0f;

  if (GetLoadingState() == ezResourceState::Loaded)
  {
    // already loaded -> more penalty
    fPriority += 30.0f;

    // the more it could discard, the less important it is to load more of it
    fPriority += GetNumQualityLevelsDiscardable() * 10.0f;
  }
  else
  {
    const ezBitflags<ezResourceFlags> flags = GetBaseResourceFlags();

    if (flags.IsAnySet(ezResourceFlags::ResourceHasFallback))
    {
      // if the resource has a very specific fallback, it is least important to be get loaded
      fPriority += 20.0f;
    }
    else if (flags.IsAnySet(ezResourceFlags::ResourceHasTypeFallback))
    {
      // if it has at least a type fallback, it is less important to get loaded
      fPriority += 10.0f;
    }
  }

  // everything acquired in the last N seconds gets a higher priority
  // by getting the lowest penalty
  const float secondsSinceAcquire = (float)(now - GetLastAcquireTime()).GetSeconds();
  const float fTimePriority = ezMath::Min(10.0f, secondsSinceAcquire);

  return fPriority + fTimePriority;
}

void ezResource::SetPriority(ezResourcePriority priority)
{
  if (m_Priority == priority)
    return;

  m_Priority = priority;

  ezResourceEvent e;
  e.m_pResource = this;
  e.m_Type = ezResourceEvent::Type::ResourcePriorityChanged;
  ezResourceManager::BroadcastResourceEvent(e);
}

ezResourceTypeLoader* ezResource::GetDefaultResourceTypeLoader() const
{
  return ezResourceManager::GetDefaultResourceLoader();
}

void ezResource::ReportResourceIsMissing()
{
  ezLog::SeriousWarning("Missing Resource of Type '{2}': '{0}' ('{1}')", ezArgSensitive(GetResourceID(), "ResourceID"),
    ezArgSensitive(m_sResourceDescription, "ResourceDesc"), GetDynamicRTTI()->GetTypeName());
}

void ezResource::VerifyAfterCreateResource(const ezResourceLoadDesc& ld)
{
  EZ_ASSERT_DEV(ld.m_State != ezResourceState::Invalid, "CreateResource() did not return a valid resource load state");
  EZ_ASSERT_DEV(ld.m_uiQualityLevelsDiscardable != 0xFF, "CreateResource() did not fill out m_uiQualityLevelsDiscardable correctly");
  EZ_ASSERT_DEV(ld.m_uiQualityLevelsLoadable != 0xFF, "CreateResource() did not fill out m_uiQualityLevelsLoadable correctly");

  IncResourceChangeCounter();

  m_LoadingState = ld.m_State;
  m_uiQualityLevelsDiscardable = ld.m_uiQualityLevelsDiscardable;
  m_uiQualityLevelsLoadable = ld.m_uiQualityLevelsLoadable;

  /* Update Memory Usage*/
  {
    ezResource::MemoryUsage MemUsage;
    MemUsage.m_uiMemoryCPU = 0xFFFFFFFF;
    MemUsage.m_uiMemoryGPU = 0xFFFFFFFF;
    UpdateMemoryUsage(MemUsage);

    EZ_ASSERT_DEV(MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its CPU memory usage", GetResourceID());
    EZ_ASSERT_DEV(MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its GPU memory usage", GetResourceID());

    m_MemoryUsage = MemUsage;
  }

  ezResourceEvent e;
  e.m_pResource = this;
  e.m_Type = ezResourceEvent::Type::ResourceContentUpdated;
  ezResourceManager::BroadcastResourceEvent(e);

  ezLog::Debug("Created {0} - '{1}' ", GetDynamicRTTI()->GetTypeName(), ezArgSensitive(GetResourceDescription(), "ResourceDesc"));
}

EZ_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_Resource);
