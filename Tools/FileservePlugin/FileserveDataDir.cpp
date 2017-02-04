#include <PCH.h>
#include <FileservePlugin/FileserveDataDir.h>
#include <Foundation/Logging/Log.h>
#include <FileservePlugin/Network/NetworkInterfaceEnet.h>

bool ezDataDirectory::FileserveType::s_bEnableFileserve = true;

ezDataDirectoryType* ezDataDirectory::FileserveType::Factory(const char* szDataDirectory)
{
  if (!s_bEnableFileserve)
    return nullptr;

  ezFileserveClient::GetSingleton()->EnsureConnected();
  return ezFileserveClient::GetSingleton()->MountDataDirectory(szDataDirectory);
}
