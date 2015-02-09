#include <PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <Core/ResourceManager/Resource.h>

static void ResourceManagerEventHandler(const ezResourceBase::ResourceEvent& e)
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  ezTelemetryMessage Msg;

  Msg.SetMessageID('RESM', 'UPDT');

  Msg.GetWriter() << static_cast<ezUInt8>(e.m_EventType);
  Msg.GetWriter() << e.m_pResource->GetResourceID();
  Msg.GetWriter() << e.m_pResource->GetDynamicRTTI()->GetTypeName();
  Msg.GetWriter() << static_cast<ezUInt8>(e.m_pResource->GetPriority());
  Msg.GetWriter() << static_cast<ezUInt8>(e.m_pResource->GetBaseResourceFlags().GetValue());
  Msg.GetWriter() << static_cast<ezUInt8>(e.m_pResource->GetLoadingState());
  Msg.GetWriter() << e.m_pResource->GetNumQualityLevelsDiscardable();
  Msg.GetWriter() << e.m_pResource->GetNumQualityLevelsLoadable();
  Msg.GetWriter() << e.m_pResource->GetMemoryUsage().m_uiMemoryCPU;
  Msg.GetWriter() << e.m_pResource->GetMemoryUsage().m_uiMemoryGPU;

  ezTelemetry::Broadcast(ezTelemetry::Reliable, Msg);
}

void AddResourceManagerEventHandler()
{
  ezResourceBase::s_Event.AddEventHandler(ResourceManagerEventHandler);
}

void RemoveResourceManagerEventHandler()
{
  ezResourceBase::s_Event.RemoveEventHandler(ResourceManagerEventHandler);
}



