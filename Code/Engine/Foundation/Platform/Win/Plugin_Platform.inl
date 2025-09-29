#include <Foundation/Configuration/Plugin.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#include <Foundation/Strings/StringBuilder.h>

using ezPluginModule = HMODULE;

bool ezPlugin::PlatformNeedsPluginCopy()
{
  return true;
}

void ezPlugin::GetPluginPaths(ezStringView sPluginName, ezStringBuilder& ref_sOriginalFile, ezStringBuilder& ref_sCopiedFile, ezUInt8 uiFileCopyNumber)
{
  ref_sOriginalFile = ezOSFile::GetApplicationDirectory();
  ref_sOriginalFile.AppendPath(sPluginName);
  ref_sOriginalFile.Append(".dll");

  ref_sCopiedFile = ezOSFile::GetApplicationDirectory();
  ref_sCopiedFile.AppendPath(sPluginName);

  if (!ezOSFile::ExistsFile(ref_sOriginalFile))
  {
    ref_sOriginalFile = ezOSFile::GetCurrentWorkingDirectory();
    ref_sOriginalFile.AppendPath(sPluginName);
    ref_sOriginalFile.Append(".dll");

    ref_sCopiedFile = ezOSFile::GetCurrentWorkingDirectory();
    ref_sCopiedFile.AppendPath(sPluginName);
  }

  if (uiFileCopyNumber > 0)
    ref_sCopiedFile.AppendFormat("{0}", uiFileCopyNumber);

  ref_sCopiedFile.Append(".loaded");
}

ezResult UnloadPluginModule(ezPluginModule& ref_pModule, ezStringView sPluginFile)
{
  // reset last error code
  SetLastError(ERROR_SUCCESS);

  if (FreeLibrary(ref_pModule) == FALSE)
  {
    ezLog::Error("Could not unload plugin '{0}'. Error-Code {1}", sPluginFile, ezArgErrorCode(GetLastError()));
    return EZ_FAILURE;
  }

  ref_pModule = nullptr;
  return EZ_SUCCESS;
}

ezResult LoadPluginModule(ezStringView sFileToLoad, ezPluginModule& ref_pModule, ezStringView sPluginFile)
{
  // reset last error code
  SetLastError(ERROR_SUCCESS);

  ref_pModule = LoadLibraryW(ezStringWChar(sFileToLoad).GetData());

  if (ref_pModule == nullptr)
  {
    const DWORD err = GetLastError();
    ezLog::Error("Could not load plugin '{0}'. Error-Code {1}", sPluginFile, ezArgErrorCode(err));

    if (err == 126)
    {
      ezLog::Error("Please Note: This means that the plugin exists, but a DLL dependency of the plugin is missing. You probably need to copy 3rd "
                   "party DLLs next to the plugin.");
    }

    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}
