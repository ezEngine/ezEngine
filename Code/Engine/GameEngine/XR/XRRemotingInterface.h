#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Configuration/CVar.h>

struct ezXRRemotingConnectionState
{
  using StorageType = ezUInt8;
  enum Enum : ezUInt8
  {
    Disconnected,
    Connecting,
    Connected,
    Default = Disconnected
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezXRRemotingConnectionState);

struct ezXRRemotingDisconnectReason
{
  using StorageType = ezUInt8;
  enum Enum : ezUInt8
  {
    None = 0,
    Unknown = 1,
    NoServerCertificate = 2,
    HandshakePortBusy = 3,
    HandshakeUnreachable = 4,
    HandshakeConnectionFailed = 5,
    AuthenticationFailed = 6,
    RemotingVersionMismatch = 7,
    IncompatibleTransportProtocols = 8,
    HandshakeFailed = 9,
    TransportPortBusy = 10,
    TransportUnreachable = 11,
    TransportConnectionFailed = 12,
    ProtocolVersionMismatch = 13,
    ProtocolError = 14,
    VideoCodecNotAvailable = 15,
    Canceled = 16,
    ConnectionLost = 17,
    DeviceLost = 18,
    DisconnectRequest = 19,
    HandshakeNetworkUnreachable = 20,
    HandshakeConnectionRefused = 21,
    VideoFormatNotAvailable = 22,
    PeerDisconnectRequest = 23,
    PeerDisconnectTimeout = 24,
    SessionOpenTimeout = 25,
    RemotingHandshakeTimeout = 26,
    InternalError = 27,
    Default = None
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezXRRemotingDisconnectReason);

struct ezXRRemotingConnectionEventData
{
  ezEnum<ezXRRemotingConnectionState> m_connectionState;
  ezEnum<ezXRRemotingDisconnectReason> m_disconnectReason;
};

typedef ezEvent<const ezXRRemotingConnectionEventData&> ezXRRemotingConnectionEvent;

/// \brief XR Remoting singleton interface. Allows for streaming the XR application to a remote device.
///
/// Needs to be initialized before ezXRInterface to be able to use remoting.
class ezXRRemotingInterface
{
public:
  /// \brief Enable XR Remoting if available.
  static ezCVarBool s_CVarXrRemoting;
  /// \brief Hostname to connect to for XR Remoting.
  static ezCVarString s_CVarXrRemotingHostName;

  /// \brief Initializes the XR Remoting system. Needs to be done before ezXRInterface is initialized.
  virtual ezResult Initialize() = 0;
  /// \brief Shuts down XR Remoting. This will fail if XR actors still exists or if ezXRInterface is still initialized.
  virtual ezResult Deinitialize() = 0;
  /// \brief Returns whether XR Remoting is initialized.
  virtual bool IsInitialized() const = 0;

  /// \name Connection Functions
  ///@{

  /// \brief Tries to connect to the remote device.
  virtual ezResult Connect(const char* remoteHostName, uint16_t remotePort = 8265, bool enableAudio = true, int maxBitrateKbps = 20000) = 0;
  /// \brief Disconnects from the remote device.
  virtual ezResult Disconnect() = 0;
  /// \brief Get the current connection state to the remote device.
  virtual ezEnum<ezXRRemotingConnectionState> GetConnectionState() const = 0;
  /// \brief Returns the connection event to subscribe to connection changes.
  virtual ezXRRemotingConnectionEvent& GetConnectionEvent() = 0;
  ///@}
};
