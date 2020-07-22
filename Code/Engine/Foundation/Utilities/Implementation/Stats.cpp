#include <FoundationPCH.h>

#include <Foundation/Utilities/Stats.h>

ezMutex ezStats::s_Mutex;
ezStats::MapType ezStats::s_Stats;
ezStats::ezEventStats ezStats::s_StatsEvents;

void ezStats::RemoveStat(const char* szStatName)
{
  EZ_LOCK(s_Mutex);

  MapType::Iterator it = s_Stats.Find(szStatName);

  if (!it.IsValid())
    return;

  s_Stats.Remove(it);

  StatsEventData e;
  e.m_EventType = StatsEventData::Remove;
  e.m_szStatName = szStatName;

  s_StatsEvents.Broadcast(e);
}

void ezStats::SetStat(const char* szStatName, const ezVariant& value)
{
  EZ_LOCK(s_Mutex);

  bool bExisted = false;
  auto it = s_Stats.FindOrAdd(szStatName, &bExisted);

  if (it.Value() == value)
    return;

  it.Value() = value;

  StatsEventData e;
  e.m_EventType = bExisted ? StatsEventData::Set : StatsEventData::Add;
  e.m_szStatName = szStatName;
  e.m_NewStatValue = value;

  s_StatsEvents.Broadcast(e);
}


EZ_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_Stats);
