#pragma once

#include <FileservePlugin/Plugin.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <ThirdParty/enet/enet.h>

/// \brief An implementation for ezRemoteInterface built on top of Enet
class EZ_FILESERVEPLUGIN_DLL ezNetworkInterfaceEnet : public ezRemoteInterface
{
  /// \brief The port through which the connection was started
  ezUInt16 GetPort() const { return m_uiPort; }

protected:
  virtual void InternalUpdateRemoteInterface() override;
  virtual ezResult InternalCreateConnection(ezRemoteMode mode, const char* szServerAddress) override;
  virtual void InternalShutdownConnection() override;
  virtual ezTime InternalGetPingToServer() override;
  virtual ezResult InternalTransmit(ezRemoteTransmitMode tm, const ezArrayPtr<const ezUInt8>& data) override;

private:
  ezUInt16 m_uiPort = 0;

  ENetAddress m_EnetServerAddress;
  ENetHost* m_pEnetHost = nullptr;
  ENetPeer* m_pEnetConnectionToServer = nullptr;
  bool m_bAllowNetworkUpdates = true;
  ezMap<void*, ezUInt32> m_EnetPeerToClientID;

  static bool s_bEnetInitialized;
};


