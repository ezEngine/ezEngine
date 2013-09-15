#include <Foundation/PCH.h>
#include <Foundation/Utilities/Stats.h>
#include <Foundation/Communication/Telemetry.h>

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
