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

  /// \brief Searches all data directories of \a client for the requested file. On success the full path is returned.
  ezResult FindFileInDataDirs(ezFileserveClientContext &client, const char* szRequestedFile, ezStringBuilder& out_sRelPath, ezStringBuilder& out_sAbsPath, ezFileserveClientContext::DataDir** ppDataDir);

  void StartServer();
  void StopServer();

  bool UpdateServer();

private:

  ezHashTable<ezUInt32, ezFileserveClientContext> m_Clients;
  ezUniquePtr<ezNetworkInterface> m_Network;
};

