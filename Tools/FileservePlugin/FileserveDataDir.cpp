#include <PCH.h>
#include <FileservePlugin/FileserveDataDir.h>
#include <Foundation/Logging/Log.h>
#include <FileservePlugin/Network/NetworkInterfaceEnet.h>

bool ezDataDirectory::FileserveType::s_bEnableFileserve = true;

ezUniquePtr<ezNetworkInterface> ezDataDirectory::FileserveType::s_Network;

ezDataDirectoryType* ezDataDirectory::FileserveType::Factory(const char* szDataDirectory)
{
  if (!s_bEnableFileserve)
    return nullptr;

  ezLog::Warning("Fileserve! {0}", szDataDirectory);

  if (s_Network == nullptr)
  {
    s_Network = EZ_DEFAULT_NEW(ezNetworkInterfaceEnet);
    s_Network->ConnectToServer('EZFS', "localhost:1042");

    ezUInt32 uiMaxRounds = 100;
    while (!s_Network->IsConnectedToServer() && uiMaxRounds > 0)
    {
      --uiMaxRounds;
      s_Network->UpdateNetwork();
      ezThreadUtils::Sleep(100);
    }

    if (uiMaxRounds == 0)
      ezLog::Error("Connection to ezFileserver timed out");
    else
      ezLog::Info("Connected to ezFileserver");
  }

  return nullptr;
}


