#include <Foundation/PCH.h>
#include <Foundation/Utilities/Stats.h>
#include <Foundation/Communication/Telemetry.h>

ezStats::MapType ezStats::s_Stats;

void ezStats::RemoveStat(const char* szStatName)
{
  MapType::Iterator it = s_Stats.Find(szStatName);

  if (!it.IsValid())
    return;

  s_Stats.Erase(it);

  if (!ezTelemetry::IsConnectedToClient())
    return;

  ezTelemetryMessage msg;
  msg.SetMessageID('STAT', 'DEL');
  msg.GetWriter() << szStatName;

  ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
}

void ezStats::SetStat(const char* szStatName, const char* szValue)
{
  ezString& sValue = s_Stats[szStatName];
  
  if (sValue == szValue)
    return;

  sValue = szValue;

  if (!ezTelemetry::IsConnectedToClient())
    return;

  ezTelemetryMessage msg;
  msg.SetMessageID('STAT', 'SET');
  msg.GetWriter() << szStatName;
  msg.GetWriter() << szValue;

  ezTelemetry::Broadcast(ezTelemetry::Unreliable, msg);
}

void ezStats::SendAllStatsTelemetry()
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  for (MapType::Iterator it = s_Stats.GetIterator(); it.IsValid(); ++it)
  {
    ezTelemetryMessage msg;
    msg.SetMessageID('STAT', 'SET');
    msg.GetWriter() << it.Key().GetData();
    msg.GetWriter() << it.Value().GetData();

    ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
  }
}

