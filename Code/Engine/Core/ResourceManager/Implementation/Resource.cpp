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
  m_DueDate = ezTime::Seconds(60.0 * 60.0 * 24.0 * 365.0 * 1000.0);
}

void ezResource::SetResourceDescription(const char* szDescription)
{
  m_sResourceDescription = szDescription;
}

void ezResource::SetDueDate(ezTime date /* = ezTime::Seconds(60.0 * 60.0 * 24.0 * 365.0 * 1000.0) */)
{
  if (m_DueDate != date)
  {
    m_DueDate = date;
  }
}

void ezResource::SetPriority(ezResourcePriority priority)
{
  if (m_Priority != priority)
  {
    m_Priority = priority;

    ezResourceEvent e;
    e.m_pResource = this;
    e.m_Type = ezResourceEvent::Type::ResourcePriorityChanged;
    ezResourceManager::BroadcastResourceEvent(e);
  }
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

  ezLog::Debug("Updated {0} - '{1}'({2}, {3}) ", GetDynamicRTTI()->GetTypeName(), GetResourceDescription(), (int)GetPriority(),
               GetLoadingDeadline(ezTime::Now()).GetSeconds());
}

ezTime ezResource::GetLoadingDeadline(ezTime tNow) const
{
  ezTime DueDate = tNow;

  ezTime tDelay;

  if (GetLoadingState() != ezResourceState::Loaded)
  {
    if (!GetBaseResourceFlags().IsAnySet(ezResourceFlags::ResourceHasFallback))
    {
      DueDate = ezTime::Seconds(0.0);
      tDelay = ezTime::Seconds(1.0); // to get a sorting by priority
    }
    else
    {
      tDelay += ezMath::Min((tNow - GetLastAcquireTime()) / 10.0, ezTime::Seconds(10.0));
    }
  }
  else
  {
    tDelay += ezTime::Seconds(GetNumQualityLevelsDiscardable() * 10.0);

    tDelay += (tNow - GetLastAcquireTime()) * 2.0;
  }

  DueDate += tDelay * ((double)GetPriority() + 1.0);

  return ezMath::Min(DueDate, m_DueDate);
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

