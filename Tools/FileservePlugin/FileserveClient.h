#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <FileservePlugin/Network/NetworkInterface.h>
#include <Foundation/Configuration/Singleton.h>

class ezFileserveClient
{
  EZ_DECLARE_SINGLETON(ezFileserveClient);

public:
  ezFileserveClient();
  ~ezFileserveClient();

  void EnsureConnected();

  void UpdateClient();

  class ezDataDirectoryType* MountDataDirectory(const char* szDataDir);

  void NetworkMsgHandler(ezNetworkMessage& msg);

private:

  ezUniquePtr<ezNetworkInterface> m_Network;
};

