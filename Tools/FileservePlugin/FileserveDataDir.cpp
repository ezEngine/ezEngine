#include <PCH.h>
#include <FileservePlugin/FileserveDataDir.h>
#include <Foundation/Logging/Log.h>

ezDataDirectoryType* ezDataDirectory::FileserveType::Factory(const char* szDataDirectory)
{
  ezLog::Warning("Fileserve! {0}", szDataDirectory);
  return nullptr;
}

