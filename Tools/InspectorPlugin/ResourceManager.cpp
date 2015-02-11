#include <PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceManager.h>

static void SendFullResourceInfo(const ezResourceBase* pRes)
{
  ezTelemetryMessage Msg;

  Msg.SetMessageID('RESM', 'SET');

  const ezTempHashedString sNameHash(pRes->GetResourceID().GetData());

  Msg.GetWriter() << sNameHash.GetHash();
  Msg.GetWriter() << pRes->GetResourceID();
  Msg.GetWriter() << pRes->GetDynamicRTTI()->GetTypeName();
  Msg.GetWriter() << static_cast<ezUInt8>(pRes->GetPriority());
  Msg.GetWriter() << static_cast<ezUInt8>(pRes->GetBaseResourceFlags().GetValue());
  Msg.GetWriter() << static_cast<ezUInt8>(pRes->GetLoadingState());
  Msg.GetWriter() << pRes->GetNumQualityLevelsDiscardable();
  Msg.GetWriter() << pRes->GetNumQualityLevelsLoadable();
  Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryCPU;
  Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryGPU;

  ezTelemetry::Broadcast(ezTelemetry::Reliable, Msg);
}

static void SendSmallResourceInfo(const ezResourceBase* pRes)
{
  ezTelemetryMessage Msg;

  Msg.SetMessageID('RESM', 'UPDT');

  const ezTempHashedString sNameHash(pRes->GetResourceID().GetData());

  Msg.GetWriter() << sNameHash.GetHash();
  Msg.GetWriter() << static_cast<ezUInt8>(pRes->GetPriority());
  Msg.GetWriter() << static_cast<ezUInt8>(pRes->GetBaseResourceFlags().GetValue());
  Msg.GetWriter() << static_cast<ezUInt8>(pRes->GetLoadingState());
  Msg.GetWriter() << pRes->GetNumQualityLevelsDiscardable();
  Msg.GetWriter() << pRes->GetNumQualityLevelsLoadable();
  Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryCPU;
  Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryGPU;

  ezTelemetry::Broadcast(ezTelemetry::Reliable, Msg);
}

static void SendDeleteResourceInfo(const ezResourceBase* pRes)
{
  ezTelemetryMessage Msg;

  Msg.SetMessageID('RESM', 'DEL');

  const ezTempHashedString sNameHash(pRes->GetResourceID().GetData());

  Msg.GetWriter() << sNameHash.GetHash();

  ezTelemetry::Broadcast(ezTelemetry::Reliable, Msg);
}

static void SendAllResourceTelemetry()
{
  ezResourceManager::BroadcastExistsEvent();
}

static void TelemetryEventsHandler(const ezTelemetry::TelemetryEventData& e)
{
  switch (e.m_EventType)
  {
  case ezTelemetry::TelemetryEventData::ConnectedToClient:
    SendAllResourceTelemetry();
    break;
  }
}

static void ResourceManagerEventHandler(const ezResourceBase::ResourceEvent& e)
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  if (e.m_EventType == ezResourceBase::ResourceEventType::Created ||
      e.m_EventType == ezResourceBase::ResourceEventType::Exists)
  {
    SendFullResourceInfo(e.m_pResource);
    return;
  }

  if (e.m_EventType == ezResourceBase::ResourceEventType::DueDateChanged) // ignore this
    return;

  if (e.m_EventType == ezResourceBase::ResourceEventType::Deleted)
  {
    SendDeleteResourceInfo(e.m_pResource);
    return;
  }

  SendSmallResourceInfo(e.m_pResource);
}

void AddResourceManagerEventHandler()
{
  ezTelemetry::AddEventHandler(TelemetryEventsHandler);
  ezResourceBase::s_Event.AddEventHandler(ResourceManagerEventHandler);
}

void RemoveResourceManagerEventHandler()
{
  ezResourceBase::s_Event.RemoveEventHandler(ResourceManagerEventHandler);
  ezTelemetry::RemoveEventHandler(TelemetryEventsHandler);
}



