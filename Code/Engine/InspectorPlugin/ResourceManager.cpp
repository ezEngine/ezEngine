#include <PCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Communication/Telemetry.h>

namespace ResourceManagerDetail
{

  static void SendFullResourceInfo(const ezResource* pRes)
  {
    ezTelemetryMessage Msg;

    Msg.SetMessageID('RESM', ' SET');

    Msg.GetWriter() << pRes->GetResourceIDHash();
    Msg.GetWriter() << pRes->GetResourceID();
    Msg.GetWriter() << pRes->GetDynamicRTTI()->GetTypeName();
    Msg.GetWriter() << static_cast<ezUInt8>(pRes->GetPriority());
    Msg.GetWriter() << static_cast<ezUInt8>(pRes->GetBaseResourceFlags().GetValue());
    Msg.GetWriter() << static_cast<ezUInt8>(pRes->GetLoadingState());
    Msg.GetWriter() << pRes->GetNumQualityLevelsDiscardable();
    Msg.GetWriter() << pRes->GetNumQualityLevelsLoadable();
    Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryCPU;
    Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryGPU;
    Msg.GetWriter() << pRes->GetResourceDescription();

    ezTelemetry::Broadcast(ezTelemetry::Reliable, Msg);
  }

  static void SendSmallResourceInfo(const ezResource* pRes)
  {
    ezTelemetryMessage Msg;

    Msg.SetMessageID('RESM', 'UPDT');

    Msg.GetWriter() << pRes->GetResourceIDHash();
    Msg.GetWriter() << static_cast<ezUInt8>(pRes->GetPriority());
    Msg.GetWriter() << static_cast<ezUInt8>(pRes->GetBaseResourceFlags().GetValue());
    Msg.GetWriter() << static_cast<ezUInt8>(pRes->GetLoadingState());
    Msg.GetWriter() << pRes->GetNumQualityLevelsDiscardable();
    Msg.GetWriter() << pRes->GetNumQualityLevelsLoadable();
    Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryCPU;
    Msg.GetWriter() << pRes->GetMemoryUsage().m_uiMemoryGPU;

    ezTelemetry::Broadcast(ezTelemetry::Reliable, Msg);
  }

  static void SendDeleteResourceInfo(const ezResource* pRes)
  {
    ezTelemetryMessage Msg;

    Msg.SetMessageID('RESM', ' DEL');

    Msg.GetWriter() << pRes->GetResourceIDHash();

    ezTelemetry::Broadcast(ezTelemetry::Reliable, Msg);
  }

  static void SendAllResourceTelemetry() { ezResourceManager::BroadcastExistsEvent(); }

  static void TelemetryEventsHandler(const ezTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case ezTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllResourceTelemetry();
        break;

      default:
        break;
    }
  }

  static void ResourceManagerEventHandler(const ezResourceEvent& e)
  {
    if (!ezTelemetry::IsConnectedToClient())
      return;

    switch (e.m_Type)
    {
      case ezResourceEvent::Type::ResourceCreated:
      case ezResourceEvent::Type::ResourceExists:
        SendFullResourceInfo(e.m_pResource);
        return;

      case ezResourceEvent::Type::ResourceDeleted:
        SendDeleteResourceInfo(e.m_pResource);
        return;

      case ezResourceEvent::Type::ResourceContentUpdated:
      case ezResourceEvent::Type::ResourceContentUnloading:
      case ezResourceEvent::Type::ResourcePriorityChanged:
        SendSmallResourceInfo(e.m_pResource);
        return;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }
} // namespace ResourceManagerDetail

void AddResourceManagerEventHandler()
{
  ezTelemetry::AddEventHandler(ResourceManagerDetail::TelemetryEventsHandler);
  ezResourceManager::s_ResourceEvents.AddEventHandler(ResourceManagerDetail::ResourceManagerEventHandler);
}

void RemoveResourceManagerEventHandler()
{
  ezResourceManager::s_ResourceEvents.RemoveEventHandler(ResourceManagerDetail::ResourceManagerEventHandler);
  ezTelemetry::RemoveEventHandler(ResourceManagerDetail::TelemetryEventsHandler);
}
