#include <CorePCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Profiling/Profiling.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezResource, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

EZ_CORE_DLL void IncreaseResourceRefCount(ezResource* pResource)
{
  pResource->m_iReferenceCount.Increment();
}

EZ_CORE_DLL void DecreaseResourceRefCount(ezResource* pResource)
{
  pResource->m_iReferenceCount.Decrement();
}

ezResource::~ezResource() = default;

ezResource::ezResource(DoUpdate ResourceUpdateThread, ezUInt8 uiQualityLevelsLoadable)
{
  m_Flags.AddOrRemove(ezResourceFlags::UpdateOnMainThread, ResourceUpdateThread == DoUpdate::OnMainThread);

  m_uiQualityLevelsLoadable = uiQualityLevelsLoadable;
}

void ezResource::SetResourceDescription(const char* szDescription)
{
  m_sResourceDescription = szDescription;
}

void ezResource::SetUniqueID(const char* szUniqueID, bool bIsReloadable)
{
  m_UniqueID = szUniqueID;
  m_uiUniqueIDHash = ezHashingUtils::xxHash32(szUniqueID, ezStringUtils::GetStringElementCount(szUniqueID));
  SetIsReloadable(bIsReloadable);

  ezResourceEvent e;
  e.m_pResource = this;
  e.m_Type = ezResourceEvent::Type::ResourceCreated;
  ezResourceManager::BroadcastResourceEvent(e);
}

void ezResource::CallUnloadData(Unload WhatToUnload)
{
  EZ_LOG_BLOCK("ezResource::UnloadData", GetResourceID().GetData());

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

void ezResource::CallUpdateContent(ezStreamReader* Stream)
{
  EZ_PROFILE_SCOPE("CallUpdateContent");

  EZ_LOG_BLOCK("ezResource::UpdateContent", GetResourceID().GetData());

  ezResourceLoadDesc ld = UpdateContent(Stream);

  EZ_ASSERT_DEV(ld.m_State != ezResourceState::Invalid, "UpdateContent() did not return a valid resource load state");
  EZ_ASSERT_DEV(ld.m_uiQualityLevelsDiscardable != 0xFF, "UpdateContent() did not fill out m_uiQualityLevelsDiscardable correctly");
  EZ_ASSERT_DEV(ld.m_uiQualityLevelsLoadable != 0xFF, "UpdateContent() did not fill out m_uiQualityLevelsLoadable correctly");

  if (ld.m_State == ezResourceState::LoadedResourceMissing)
  {
    ReportResourceIsMissing();
  }

  IncResourceChangeCounter();

  m_LoadingState = ld.m_State;
  m_uiQualityLevelsDiscardable = ld.m_uiQualityLevelsDiscardable;
  m_uiQualityLevelsLoadable = ld.m_uiQualityLevelsLoadable;

  ezResourceEvent e;
  e.m_pResource = this;
  e.m_Type = ezResourceEvent::Type::ResourceContentUpdated;
  ezResourceManager::BroadcastResourceEvent(e);

  ezLog::Debug("Updated {0} - '{1}'", GetDynamicRTTI()->GetTypeName(), GetResourceDescription());
}

float ezResource::GetLoadingPriority(ezTime tNow) const
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
  const float secondsSinceAcquire = (float)(tNow - GetLastAcquireTime()).GetSeconds();
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
  ezLog::Error("Missing Resource of Type '{2}': '{0}' ('{1}')", GetResourceID(), m_sResourceDescription, GetDynamicRTTI()->GetTypeName());
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

  ezLog::Debug("Created {0} - '{1}' ", GetDynamicRTTI()->GetTypeName(), GetResourceDescription());
}

EZ_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_Resource);
