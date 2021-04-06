#include <GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/Declarations.h>
#include <GameEngine/XR/XRRemotingInterface.h>

ezCVarBool ezXRRemotingInterface::s_CVarXrRemoting("xr_Remoting", false, ezCVarFlags::Default, "Enable XR Remoting if available.");
ezCVarString ezXRRemotingInterface::s_CVarXrRemotingHostName("xr_RemotingHostName", "", ezCVarFlags::Save, "Hostname to connect to for XR Remoting.");

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezXRTransformSpace, 1)
  EZ_BITFLAGS_CONSTANTS(ezXRTransformSpace::Local, ezXRTransformSpace::Global)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezXRDeviceType, 1)
  EZ_BITFLAGS_CONSTANTS(ezXRDeviceType::HMD, ezXRDeviceType::LeftController, ezXRDeviceType::RightController)
  EZ_BITFLAGS_CONSTANTS(ezXRDeviceType::DeviceID0, ezXRDeviceType::DeviceID1, ezXRDeviceType::DeviceID2, ezXRDeviceType::DeviceID3)
  EZ_BITFLAGS_CONSTANTS(ezXRDeviceType::DeviceID4, ezXRDeviceType::DeviceID5, ezXRDeviceType::DeviceID6, ezXRDeviceType::DeviceID7)
  EZ_BITFLAGS_CONSTANTS(ezXRDeviceType::DeviceID8, ezXRDeviceType::DeviceID9, ezXRDeviceType::DeviceID10, ezXRDeviceType::DeviceID11)
  EZ_BITFLAGS_CONSTANTS(ezXRDeviceType::DeviceID12, ezXRDeviceType::DeviceID13, ezXRDeviceType::DeviceID14, ezXRDeviceType::DeviceID15)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezXRRemotingConnectionState, 1)
  EZ_BITFLAGS_CONSTANTS(ezXRRemotingConnectionState::Disconnected, ezXRRemotingConnectionState::Connecting, ezXRRemotingConnectionState::Connected)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezXRRemotingDisconnectReason, 1)
  EZ_BITFLAGS_CONSTANTS(ezXRRemotingDisconnectReason::None, ezXRRemotingDisconnectReason::Unknown, ezXRRemotingDisconnectReason::NoServerCertificate)
  EZ_BITFLAGS_CONSTANTS(ezXRRemotingDisconnectReason::HandshakePortBusy, ezXRRemotingDisconnectReason::HandshakeUnreachable, ezXRRemotingDisconnectReason::HandshakeConnectionFailed)
  EZ_BITFLAGS_CONSTANTS(ezXRRemotingDisconnectReason::AuthenticationFailed, ezXRRemotingDisconnectReason::RemotingVersionMismatch, ezXRRemotingDisconnectReason::IncompatibleTransportProtocols)
  EZ_BITFLAGS_CONSTANTS(ezXRRemotingDisconnectReason::HandshakeFailed, ezXRRemotingDisconnectReason::TransportPortBusy, ezXRRemotingDisconnectReason::TransportUnreachable)
  EZ_BITFLAGS_CONSTANTS(ezXRRemotingDisconnectReason::TransportConnectionFailed, ezXRRemotingDisconnectReason::ProtocolVersionMismatch, ezXRRemotingDisconnectReason::ProtocolError)
  EZ_BITFLAGS_CONSTANTS(ezXRRemotingDisconnectReason::VideoCodecNotAvailable, ezXRRemotingDisconnectReason::Canceled, ezXRRemotingDisconnectReason::ConnectionLost)
  EZ_BITFLAGS_CONSTANTS(ezXRRemotingDisconnectReason::DeviceLost, ezXRRemotingDisconnectReason::DisconnectRequest, ezXRRemotingDisconnectReason::HandshakeNetworkUnreachable)
  EZ_BITFLAGS_CONSTANTS(ezXRRemotingDisconnectReason::HandshakeConnectionRefused, ezXRRemotingDisconnectReason::VideoFormatNotAvailable, ezXRRemotingDisconnectReason::PeerDisconnectRequest)
  EZ_BITFLAGS_CONSTANTS(ezXRRemotingDisconnectReason::PeerDisconnectTimeout, ezXRRemotingDisconnectReason::SessionOpenTimeout, ezXRRemotingDisconnectReason::RemotingHandshakeTimeout)
  EZ_BITFLAGS_CONSTANTS(ezXRRemotingDisconnectReason::InternalError)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezXRDeviceState::ezXRDeviceState()
{
  m_vGripPosition.SetZero();
  m_qGripRotation.SetIdentity();

  m_vAimPosition.SetZero();
  m_qAimRotation.SetIdentity();
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_Declaration);
