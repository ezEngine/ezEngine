#pragma once

#include <FileservePlugin/Fileserver/ClientContext.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Configuration/Singleton.h>
#include <FileservePlugin/Network/NetworkInterface.h>
#include <Foundation/Types/UniquePtr.h>

class ezNetworkMessage;

class EZ_FILESERVEPLUGIN_DLL ezFileserver
{
  EZ_DECLARE_SINGLETON(ezFileserver);

public:
  ezFileserver();

  void NetworkMsgHandler(ezNetworkMessage& msg);

  void HandleMountRequest(ezFileserveClientContext& client, ezNetworkMessage &msg);

  void HandleFileRequest(ezFileserveClientContext& client, ezNetworkMessage &msg);



  void StartServer();
  void StopServer();

  bool UpdateServer();

private:

  ezHashTable<ezUInt32, ezFileserveClientContext> m_Clients;
  ezUniquePtr<ezNetworkInterface> m_Network;
};

