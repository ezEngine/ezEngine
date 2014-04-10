#include <Core/PCH.h>
#include <Core/ResourceManager/Resource.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezResourceBase, ezNoBase, ezRTTINoAllocator);

EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTime ezResourceBase::GetLoadingDeadline(ezTime tNow) const
{
  ezTime DueDate = tNow;

  if (m_LoadingState == ezResourceLoadState::Uninitialized)
  {
    if (!m_bHasFallback)
      return ezTime::Seconds(0.0);

    DueDate += ezTime::Seconds((double) m_Priority + 1.0);
  }
  else
  {
    if (m_uiMaxQualityLevel > 0)
    {
      double fQuality = (double) m_uiLoadedQualityLevel / (double) m_uiMaxQualityLevel;
      DueDate += ezTime::Seconds(fQuality * 5.0);
    }

    DueDate += (ezTime::Seconds(1.0) + (tNow - m_LastAcquire)) * ((double) m_Priority + 1.0);

  }

  return ezMath::Min(DueDate, m_DueDate);
}


