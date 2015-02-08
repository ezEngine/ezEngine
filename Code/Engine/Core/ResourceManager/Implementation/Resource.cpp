#include <Core/PCH.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/ResourceManager/ResourceManager.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezResourceBase, ezReflectedClass, 1, ezRTTINoAllocator);

EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_CORE_DLL void IncreaseResourceRefCount(ezResourceBase* pResource)
{
  pResource->m_iReferenceCount.Increment();
}

EZ_CORE_DLL void DecreaseResourceRefCount(ezResourceBase* pResource)
{
  pResource->m_iReferenceCount.Decrement();
}

ezResourceBase::ezResourceBase(UpdateResource ResourceUpdateThread, ezUInt8 uiQualityLevelsLoadable)
{
  m_Flags.AddOrRemove(ezResourceFlags::UpdateOnMainThread, ResourceUpdateThread == UpdateResource::OnMainThread);

  m_iReferenceCount = 0;
  m_LoadingState = ezResourceState::Unloaded;
  m_uiQualityLevelsLoadable = uiQualityLevelsLoadable;
  m_uiQualityLevelsDiscardable = 0;
  m_Priority = ezResourcePriority::Normal;
  m_bIsPreloading = false;
  SetDueDate();
}

void ezResourceBase::SetUniqueID(const ezString& UniqueID)
{
  m_UniqueID = UniqueID;
}

void ezResourceBase::CallUnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc ld = UnloadData(WhatToUnload);

  EZ_ASSERT_DEV(ld.m_State != ezResourceState::Invalid, "UnloadData() did not return a valid resource load state");
  EZ_ASSERT_DEV(ld.m_uiQualityLevelsDiscardable != 0xFF, "UnloadData() did not fill out m_uiQualityLevelsDiscardable correctly");
  EZ_ASSERT_DEV(ld.m_uiQualityLevelsLoadable != 0xFF, "UnloadData() did not fill out m_uiQualityLevelsLoadable correctly");

  m_LoadingState = ld.m_State;
  m_uiQualityLevelsDiscardable = ld.m_uiQualityLevelsDiscardable;
  m_uiQualityLevelsLoadable = ld.m_uiQualityLevelsLoadable;
}

void ezResourceBase::CallUpdateContent(ezStreamReaderBase* Stream)
{
  ezResourceLoadDesc ld = UpdateContent(Stream);

  EZ_ASSERT_DEV(ld.m_State != ezResourceState::Invalid, "UpdateContent() did not return a valid resource load state");
  EZ_ASSERT_DEV(ld.m_uiQualityLevelsDiscardable != 0xFF, "UpdateContent() did not fill out m_uiQualityLevelsDiscardable correctly");
  EZ_ASSERT_DEV(ld.m_uiQualityLevelsLoadable != 0xFF, "UpdateContent() did not fill out m_uiQualityLevelsLoadable correctly");

  m_LoadingState = ld.m_State;
  m_uiQualityLevelsDiscardable = ld.m_uiQualityLevelsDiscardable;
  m_uiQualityLevelsLoadable = ld.m_uiQualityLevelsLoadable;
}

ezTime ezResourceBase::GetLoadingDeadline(ezTime tNow) const
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

  DueDate += tDelay * ((double) GetPriority() + 1.0);

  return ezMath::Min(DueDate, m_DueDate);
}

ezResourceTypeLoader* ezResourceBase::GetDefaultResourceTypeLoader() const
{
  return ezResourceManager::GetDefaultResourceLoader();
}





EZ_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_Resource);

