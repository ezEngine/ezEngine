#include <OpenXRPlugin/OpenXRPluginPCH.h>

#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT

#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Reflection/ReflectionUtils.h>
#  include <Foundation/System/EnvironmentVariableUtils.h>
#  include <OpenXRPlugin/OpenXRDeclarations.h>
#  include <OpenXRPlugin/OpenXRRemoting.h>
#  include <OpenXRPlugin/OpenXRSingleton.h>

EZ_IMPLEMENT_SINGLETON(ezOpenXRRemoting);

ezOpenXRRemoting::ezOpenXRRemoting(ezOpenXR* pOpenXR)
  : m_SingletonRegistrar(this)
  , m_pOpenXR(pOpenXR)
{
}

ezOpenXRRemoting::~ezOpenXRRemoting() = default;

ezResult ezOpenXRRemoting::Initialize()
{
  if (m_pOpenXR->IsInitialized())
  {
    return EZ_FAILURE;
  }

  ezStringBuilder sRemotingJson = ezOSFile::GetApplicationDirectory();
  sRemotingJson.AppendPath("RemotingXR.json");

  if (ezOSFile::ExistsFile(sRemotingJson))
  {
    m_sPreviousRuntime = ezEnvironmentVariableUtils::GetValueString("XR_RUNTIME_JSON");
    if (ezEnvironmentVariableUtils::SetValueString("XR_RUNTIME_JSON", sRemotingJson).Failed())
    {
      ezLog::Error("Failed to set environment variable XR_RUNTIME_JSON.");
      return EZ_FAILURE;
    }

    m_bInitialized = true;
    return EZ_SUCCESS;
  }
  else
  {
    ezLog::Error("XR_RUNTIME_JSON not found: {}", sRemotingJson);
    return EZ_FAILURE;
  }
}

ezResult ezOpenXRRemoting::Deinitialize()
{
  if (!m_bInitialized)
  {
    return EZ_SUCCESS;
  }

  if (m_pOpenXR->IsInitialized())
    return EZ_FAILURE;

  m_bInitialized = false;
  SetEnvironmentVariableW(L"XR_RUNTIME_JSON", ezStringWChar(m_sPreviousRuntime));
  m_sPreviousRuntime.Clear();
  return EZ_SUCCESS;
}

bool ezOpenXRRemoting::IsInitialized() const
{
  return m_bInitialized;
}

ezResult ezOpenXRRemoting::Connect(const char* szRemoteHostName, uint16_t remotePort, bool bEnableAudio, int iMaxBitrateKbps)
{
  EZ_ASSERT_DEV(IsInitialized(), "Need to call 'ezXRRemotingInterface::Initialize' first.");
  EZ_ASSERT_DEV(m_pOpenXR->IsInitialized(), "Need to call 'ezXRInterface::Initialize' first.");

  XrRemotingRemoteContextPropertiesMSFT contextProperties;
  contextProperties = XrRemotingRemoteContextPropertiesMSFT{static_cast<XrStructureType>(XR_TYPE_REMOTING_REMOTE_CONTEXT_PROPERTIES_MSFT)};
  contextProperties.enableAudio = bEnableAudio;
  contextProperties.maxBitrateKbps = iMaxBitrateKbps;
  contextProperties.videoCodec = XR_REMOTING_VIDEO_CODEC_ANY_MSFT;
  contextProperties.depthBufferStreamResolution = XR_REMOTING_DEPTH_BUFFER_STREAM_RESOLUTION_HALF_MSFT;
  XrResult res = m_pOpenXR->m_Extensions.pfn_xrRemotingSetContextPropertiesMSFT(m_pOpenXR->m_pInstance, m_pOpenXR->m_SystemId, &contextProperties);
  if (res != XrResult::XR_SUCCESS)
  {
    XR_LOG_ERROR(res);
    return EZ_FAILURE;
  }

  XrRemotingConnectInfoMSFT connectInfo{static_cast<XrStructureType>(XR_TYPE_REMOTING_CONNECT_INFO_MSFT)};
  connectInfo.remoteHostName = szRemoteHostName;
  connectInfo.remotePort = remotePort;
  connectInfo.secureConnection = false;
  res = m_pOpenXR->m_Extensions.pfn_xrRemotingConnectMSFT(m_pOpenXR->m_pInstance, m_pOpenXR->m_SystemId, &connectInfo);
  if (res != XrResult::XR_SUCCESS)
  {
    return EZ_FAILURE;
  }
  {
    ezXRRemotingConnectionEventData data;
    data.m_connectionState = ezXRRemotingConnectionState::Connecting;
    data.m_disconnectReason = ezXRRemotingDisconnectReason::None;
    m_Event.Broadcast(data);
  }
  return EZ_SUCCESS;
}

ezResult ezOpenXRRemoting::Disconnect()
{
  if (!m_bInitialized || !m_pOpenXR->IsInitialized())
    return EZ_SUCCESS;

  XrRemotingDisconnectInfoMSFT disconnectInfo;
  XrResult res = m_pOpenXR->m_Extensions.pfn_xrRemotingDisconnectMSFT(m_pOpenXR->m_pInstance, m_pOpenXR->m_SystemId, &disconnectInfo);
  if (res != XrResult::XR_SUCCESS)
  {
    return EZ_FAILURE;
  }
  return EZ_SUCCESS;
}

ezEnum<ezXRRemotingConnectionState> ezOpenXRRemoting::GetConnectionState() const
{
  if (!m_bInitialized || !m_pOpenXR->IsInitialized())
    return ezXRRemotingConnectionState::Disconnected;

  XrRemotingConnectionStateMSFT connectionState;
  XrResult res = m_pOpenXR->m_Extensions.pfn_xrRemotingGetConnectionStateMSFT(m_pOpenXR->m_pInstance, m_pOpenXR->m_SystemId, &connectionState, nullptr);
  if (res != XrResult::XR_SUCCESS)
  {
    return ezXRRemotingConnectionState::Disconnected;
  }
  switch (connectionState)
  {
    case XR_REMOTING_CONNECTION_STATE_DISCONNECTED_MSFT:
      return ezXRRemotingConnectionState::Disconnected;
    case XR_REMOTING_CONNECTION_STATE_CONNECTING_MSFT:
      return ezXRRemotingConnectionState::Connecting;
    case XR_REMOTING_CONNECTION_STATE_CONNECTED_MSFT:
      return ezXRRemotingConnectionState::Connected;
    default:
      EZ_REPORT_FAILURE("Unknown enum value");
      break;
  }

  return ezXRRemotingConnectionState::Disconnected;
}

ezXRRemotingConnectionEvent& ezOpenXRRemoting::GetConnectionEvent()
{
  return m_Event;
}

void ezOpenXRRemoting::HandleEvent(const XrEventDataBuffer& event)
{
  if (!m_bInitialized)
    return;

  switch ((ezUInt32)event.type)
  {
    case XR_TYPE_REMOTING_EVENT_DATA_CONNECTED_MSFT:
    {
      ezXRRemotingConnectionEventData data;
      data.m_connectionState = ezXRRemotingConnectionState::Connected;
      data.m_disconnectReason = ezXRRemotingDisconnectReason::None;

      ezLog::Info("XR Remoting connected.");
      m_Event.Broadcast(data);
    }
    break;
    case XR_TYPE_REMOTING_EVENT_DATA_DISCONNECTED_MSFT:
    {
      ezXRRemotingConnectionEventData data;
      data.m_connectionState = ezXRRemotingConnectionState::Disconnected;
      XrRemotingDisconnectReasonMSFT reason = reinterpret_cast<const XrRemotingEventDataDisconnectedMSFT*>(&event)->disconnectReason;
      data.m_disconnectReason = static_cast<ezXRRemotingDisconnectReason::Enum>(reason);

      ezStringBuilder sTemp;
      ezReflectionUtils::EnumerationToString(data.m_disconnectReason, sTemp, ezReflectionUtils::EnumConversionMode::ValueNameOnly);
      ezLog::Info("XR Remoting disconnected with reason: {}", sTemp);

      m_Event.Broadcast(data);
    }
    break;
    default:
      break;
  }
}

#endif
