#include <Foundation/PCH.h>
#include <Foundation/Utilities/Stats.h>

ezStats::MapType ezStats::s_Stats;
ezStats::ezEventStats ezStats::s_StatsEvents;

void ezStats::RemoveStat(const char* szStatName)
{
  MapType::Iterator it = s_Stats.Find(szStatName);

  if (!it.IsValid())
    return;

  s_Stats.Remove(it);

  StatsEventData e;
  e.m_EventType = StatsEventData::Remove;
  e.m_szStatName = szStatName;
  e.m_szNewStatValue = nullptr;

  s_StatsEvents.Broadcast(e);
}

void ezStats::SetStat(const char* szStatName, const char* szValue)
{
  bool bExisted = false;
  auto it = s_Stats.FindOrAdd(szStatName, &bExisted);

  if (it.Value() == szValue)
    return;

  it.Value() = szValue;

  StatsEventData e;
  e.m_EventType = bExisted ? StatsEventData::Set : StatsEventData::Add;
  e.m_szStatName = szStatName;
  e.m_szNewStatValue = szValue;

  s_StatsEvents.Broadcast(e);
}


EZ_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_Stats);

