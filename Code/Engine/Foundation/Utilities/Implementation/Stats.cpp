#include <Foundation/FoundationPCH.h>

#include <Foundation/Utilities/Stats.h>

#if TRACY_ENABLE
#  include <tracy/tracy/Tracy.hpp>
#endif

ezMutex ezStats::s_Mutex;
ezStats::MapType ezStats::s_Stats;
ezStats::ezEventStats ezStats::s_StatsEvents;

void ezStats::RemoveStat(ezStringView sStatName)
{
  EZ_LOCK(s_Mutex);

  MapType::Iterator it = s_Stats.Find(sStatName);

  if (!it.IsValid())
    return;

  s_Stats.Remove(it);

  StatsEventData e;
  e.m_EventType = StatsEventData::Remove;
  e.m_sStatName = sStatName;

  s_StatsEvents.Broadcast(e);
}

void ezStats::SetStat(ezStringView sStatName, const ezVariant& value)
{
  EZ_LOCK(s_Mutex);

  bool bExisted = false;
  auto it = s_Stats.FindOrAdd(sStatName, &bExisted);

  if (it.Value() == value)
    return;

  it.Value() = value;

  StatsEventData e;
  e.m_EventType = bExisted ? StatsEventData::Set : StatsEventData::Add;
  e.m_sStatName = sStatName;
  e.m_NewStatValue = value;

  s_StatsEvents.Broadcast(e);

#if TRACY_ENABLE
  if (value.IsNumber())
  {
    TracyPlot(it.Key().GetData(), value.ConvertTo<double>());
  }
#endif
}
