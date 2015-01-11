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

ezResourceBase::ezResourceBase()
{
  m_iReferenceCount = 0;
  m_LoadingState = ezResourceLoadState::Uninitialized;
  m_uiMaxQualityLevel = 0;
  m_uiLoadedQualityLevel = 0;
  m_Priority = ezResourcePriority::Normal;
  m_bIsPreloading = false;
  SetDueDate();
  m_uiMemoryCPU = 0;
  m_uiMemoryGPU = 0;
}

void ezResourceBase::SetUniqueID(const ezString& UniqueID)
{
  m_UniqueID = UniqueID;
}

ezTime ezResourceBase::GetLoadingDeadline(ezTime tNow) const
{
  ezTime DueDate = tNow;

  /// \todo This needs to be tweaked.
  /// Resources that are not loaded should ALWAYS take precedence before ALL others (unless not requested in a longer time).

  if (GetLoadingState() != ezResourceLoadState::Loaded)
  {
    if (!GetBaseResourceFlags().IsAnySet(ezResourceFlags::ResourceHasFallback))
      DueDate = ezTime::Seconds(0.0);

    DueDate += ezTime::Seconds((double) GetPriority() + 1.0);
  }
  else
  {
    if (GetMaxQualityLevel() > 0)
    {
      double fQuality = (double) GetLoadedQualityLevel() / (double) GetMaxQualityLevel();
      DueDate += ezTime::Seconds(fQuality * 5.0);
    }

    DueDate += (ezTime::Seconds(1.0) + (tNow - GetLastAcquireTime())) * ((double) GetPriority() + 1.0);

  }

  return ezMath::Min(DueDate, m_DueDate);
}

ezResourceTypeLoader* ezResourceBase::GetDefaultResourceTypeLoader() const
{
  return ezResourceManager::GetDefaultResourceLoader();
}





EZ_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_Resource);

