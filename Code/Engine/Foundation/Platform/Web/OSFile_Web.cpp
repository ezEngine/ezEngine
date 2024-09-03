#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WEB)

#  define EZ_SKIP_FOLDER_PATHS
#  include <Foundation/Platform/Posix/OSFile_Posix.h>

ezStringView ezOSFile::GetApplicationPath()
{
  s_sApplicationPath = "/";
  return s_sApplicationPath;
}

ezString ezOSFile::GetUserDataFolder(ezStringView sSubFolder)
{
  return "/web-app-user";
}

ezString ezOSFile::GetTempDataFolder(ezStringView sSubFolder)
{
  return "/web-app-temp";
}

ezString ezOSFile::GetUserDocumentsFolder(ezStringView sSubFolder)
{
  return "/web-app-docs";
}

const ezString ezOSFile::GetCurrentWorkingDirectory()
{
  return ".";
}

#endif
