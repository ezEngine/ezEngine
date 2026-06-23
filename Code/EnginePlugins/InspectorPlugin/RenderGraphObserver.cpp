#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererCore/RenderGraph/RenderGraphInspectionInfo.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderGraph/RenderGraphPassObserver.h>

namespace RenderGraphObserverDetail
{
  static constexpr ezUInt32 s_uiSystemId = 'RGPH';
  static constexpr ezUInt16 s_uiProtocolVersion = 1;

  static ezSharedPtr<ezRenderGraphPassObserver> s_pObserver;
  static ezRenderGraphInspectionSummary s_LastSummary;
  static ezRenderGraphInspectionInfo s_LastInfo;
  static ezUInt64 s_uiRequestedInfoGraph = 0;
  static ezUInt64 s_uiLastInfoGraph = 0;
  static bool s_bLastInfoValid = false;
  static bool s_bSummaryDirty = false;
  static bool s_bInfoDirty = false;
  static ezTime s_LastSummaryBroadcast;
  static ezTime s_LastResponseBroadcast;
  static bool s_bObserverResponseEnabled = false;

  static void EnsureObserver()
  {
    if (s_pObserver == nullptr)
    {
      s_pObserver = ezRenderGraphManager::CreateObserver();
    }
  }

  static void SendSummary()
  {
    ezTelemetryMessage msg;
    msg.SetMessageID(s_uiSystemId, 'SUMM');
    msg.GetWriter() << s_uiProtocolVersion;
    msg.GetWriter() << s_LastSummary;
    ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
    s_bSummaryDirty = false;
  }

  static void SendInspectionInfo(ezUInt64 uiGraphIdentity)
  {
    ezTelemetryMessage msg;
    msg.SetMessageID(s_uiSystemId, 'INFO');
    msg.GetWriter() << s_uiProtocolVersion;
    msg.GetWriter() << uiGraphIdentity;

    const bool bSuccess = s_bLastInfoValid && s_uiLastInfoGraph == uiGraphIdentity;
    msg.GetWriter() << bSuccess;
    if (bSuccess)
    {
      msg.GetWriter() << s_LastInfo;
    }

    ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
    if (uiGraphIdentity == s_uiLastInfoGraph)
    {
      s_bInfoDirty = false;
    }
  }

  static void RenderGraphRenderEventHandler(const ezRenderGraphRenderEvent& e)
  {
    if (e.m_Type != ezRenderGraphRenderEvent::Type::AfterGraphExecution)
      return;

    ezRenderGraphManager::GetExecutionSummary(s_LastSummary);
    s_bSummaryDirty = true;

    if (s_uiRequestedInfoGraph != 0)
    {
      s_bLastInfoValid = ezRenderGraphManager::GetRenderGraphInspectionInfo(s_uiRequestedInfoGraph, s_LastInfo).Succeeded();
      if (!s_bLastInfoValid)
      {
        s_LastInfo.Clear();
      }

      s_uiLastInfoGraph = s_uiRequestedInfoGraph;
      s_bInfoDirty = true;
    }
  }

  static void SendObserverResponse()
  {
    if (s_pObserver == nullptr)
      return;

    ezTelemetryMessage msg;
    msg.SetMessageID(s_uiSystemId, 'RESP');
    msg.GetWriter() << s_uiProtocolVersion;
    msg.GetWriter() << s_pObserver->GetResponse();
    ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
  }

  static void ClearObserverRequest()
  {
    if (s_pObserver == nullptr)
      return;

    ezRenderGraphObserverRequest request;
    s_pObserver->SetRequest(request);
    s_bObserverResponseEnabled = false;
  }

  static void HandleMessages(void*)
  {
    ezTelemetryMessage msg;
    while (ezTelemetry::RetrieveMessage(s_uiSystemId, msg) == EZ_SUCCESS)
    {
      ezUInt16 uiVersion = 0;

      switch (msg.GetMessageID())
      {
        case 'RSUM':
          SendSummary();
          break;

        case 'RINF':
        {
          ezUInt64 uiGraphIdentity = 0;
          msg.GetReader() >> uiVersion;
          msg.GetReader() >> uiGraphIdentity;
          s_uiRequestedInfoGraph = uiGraphIdentity;
          SendInspectionInfo(uiGraphIdentity);
        }
        break;

        case 'RREQ':
        {
          ezRenderGraphObserverRequest request;
          msg.GetReader() >> uiVersion;
          msg.GetReader() >> request;

          EnsureObserver();
          s_pObserver->SetRequest(request);
          s_bObserverResponseEnabled = request.m_uiRenderGraphId != 0 && !request.m_sPassName.IsEmpty();
        }
        break;

        case 'RCLR':
          ClearObserverRequest();
          break;

        default:
          break;
      }
    }
  }

  static void TelemetryEventsHandler(const ezTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case ezTelemetry::TelemetryEventData::ConnectedToClient:
        s_bSummaryDirty = true;
        break;

      case ezTelemetry::TelemetryEventData::DisconnectedFromClient:
        s_pObserver = nullptr;
        s_bObserverResponseEnabled = false;
        break;

      case ezTelemetry::TelemetryEventData::PerFrameUpdate:
      {
        if (!ezTelemetry::IsConnectedToClient())
          return;

        const ezTime now = ezTime::Now();
        if (s_bSummaryDirty && (now - s_LastSummaryBroadcast > ezTime::MakeFromSeconds(0.5)))
        {
          s_LastSummaryBroadcast = now;
          SendSummary();
        }

        if (s_bInfoDirty)
        {
          SendInspectionInfo(s_uiLastInfoGraph);
        }

        if (s_bObserverResponseEnabled && (now - s_LastResponseBroadcast > ezTime::MakeFromSeconds(0.1)))
        {
          s_LastResponseBroadcast = now;
          SendObserverResponse();
        }
      }
      break;

      default:
        break;
    }
  }
} // namespace RenderGraphObserverDetail

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(InspectorPlugin, RenderGraphObserver)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "RenderGraphManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    RenderGraphObserverDetail::s_pObserver = nullptr;
    RenderGraphObserverDetail::s_bObserverResponseEnabled = false;
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

void AddRenderGraphEventHandler()
{
  ezTelemetry::AcceptMessagesForSystem(RenderGraphObserverDetail::s_uiSystemId, true, RenderGraphObserverDetail::HandleMessages, nullptr);
  ezTelemetry::AddEventHandler(RenderGraphObserverDetail::TelemetryEventsHandler);
  ezRenderGraphManager::s_RenderEvent.AddEventHandler(RenderGraphObserverDetail::RenderGraphRenderEventHandler);
}

void RemoveRenderGraphEventHandler()
{
  ezRenderGraphManager::s_RenderEvent.RemoveEventHandler(RenderGraphObserverDetail::RenderGraphRenderEventHandler);
  ezTelemetry::RemoveEventHandler(RenderGraphObserverDetail::TelemetryEventsHandler);
  ezTelemetry::AcceptMessagesForSystem(RenderGraphObserverDetail::s_uiSystemId, false);
}
