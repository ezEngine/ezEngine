
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>

typedef HMODULE ezPluginModule;

void ezPlugin::GetPluginPaths(const char* szPluginName, ezStringBuilder& sOldPath, ezStringBuilder& sNewPath, ezUInt8 uiFileNumber)
{
  sOldPath = ezOSFile::GetApplicationDirectory();
  sOldPath.AppendPath(szPluginName);
  sOldPath.Append(".dll");

  sNewPath = ezOSFile::GetApplicationDirectory();
  sNewPath.AppendPath(szPluginName);

  if (uiFileNumber > 0)
    sNewPath.AppendFormat("{0}", uiFileNumber);

  sNewPath.Append(".loaded");
}

ezResult UnloadPluginModule(ezPluginModule& Module, const char* szPluginFile)
{
  // reset last error code
  if (GetLastError())
  {
  }

  if (FreeLibrary(Module) == FALSE)
  {
    ezLog::Error("Could not unload plugin '{0}'. Error-Code {1}", szPluginFile, ezArgErrorCode(GetLastError()));
    return EZ_FAILURE;
  }

  Module = nullptr;
  return EZ_SUCCESS;
}

ezResult LoadPluginModule(const char* szFileToLoad, ezPluginModule& Module, const char* szPluginFile)
{
  // reset last error code
  if (GetLastError())
  {
  }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  ezStringBuilder relativePath = szFileToLoad;
  relativePath.MakeRelativeTo(ezOSFile::GetApplicationDirectory());
  Module = LoadPackagedLibrary(ezStringWChar(relativePath).GetData(), 0);
#else
  Module = LoadLibraryW(ezStringWChar(szFileToLoad).GetData());
#endif

  if (Module == nullptr)
  {
    DWORD err = GetLastError();
    ezLog::Error("Could not load plugin '{0}'. Error-Code {1}", szPluginFile, ezArgErrorCode(err));

    if (err == 126)
    {
      ezLog::Error("Please Note: This means that the plugin exists, but a DLL dependency of the plugin is missing. You probably need to copy 3rd party DLLs next to the plugin.");
    }

    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

#else
#error "This file should not have been included."
#endif

