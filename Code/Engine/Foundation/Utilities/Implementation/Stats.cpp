#include <Foundation/PCH.h>
#include <Foundation/Utilities/Stats.h>

ezStats::MapType ezStats::s_Stats;
ezStats::ezEventStats ezStats::s_StatsEvents;

void ezStats::RemoveStat(const char* szStatName)
{
  MapType::Iterator it = s_Stats.Find(szStatName);

  if (!it.IsValid())
    return;

  s_Stats.Erase(it);

  StatsEventData e;
  e.m_EventType = StatsEventData::Remove;
  e.m_szStatName = szStatName;
  e.m_szNewStatValue = NULL;

  s_StatsEvents.Broadcast(e);
}

void ezStats::SetStat(const char* szStatName, const char* szValue)
{
  /// \todo Stats are transmitted unreliably, should be transmitted reliable when they are new.
  // Otherwise it can happen that some stat never appears, because the packet got lost at startup and then it never changed


  ezString& sValue = s_Stats[szStatName];
  
  if (sValue == szValue)
    return;

  sValue = szValue;

  StatsEventData e;
  e.m_EventType = StatsEventData::Set;
  e.m_szStatName = szStatName;
  e.m_szNewStatValue = szValue;

  s_StatsEvents.Broadcast(e);
}


EZ_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_Stats);

