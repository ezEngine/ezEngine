#pragma once

#include <FileservePlugin/Network/NetworkInterface.h>
#include <ThirdParty/enet/enet.h>

/// \brief An implementation for ezNetworkInterface built on top of Enet
class EZ_FILESERVEPLUGIN_DLL ezNetworkInterfaceEnet : public ezNetworkInterface
{
protected:
  virtual void InternalUpdateNetwork() override;
  virtual ezResult InternalCreateConnection(ezNetworkMode mode, ezUInt16 uiPort, const char* szServerAddress) override;
  virtual void InternalShutdownConnection() override;
  virtual ezTime InternalGetPingToServer() override;
  virtual ezResult InternalTransmit(ezNetworkTransmitMode tm, const ezArrayPtr<const ezUInt8>& data) override;

private:
  ENetAddress m_EnetServerAddress;
  ENetHost* m_pEnetHost = nullptr;
  ENetPeer* m_pEnetConnectionToServer = nullptr;
  bool m_bAllowNetworkUpdates = true;

  static bool s_bEnetInitialized;
};


