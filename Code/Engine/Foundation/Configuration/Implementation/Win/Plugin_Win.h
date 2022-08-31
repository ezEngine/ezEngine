
#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Configuration/Plugin.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Strings/StringBuilder.h>

typedef HMODULE ezPluginModule;

void ezPlugin::GetPluginPaths(const char* szPluginName, ezStringBuilder& sOriginalFile, ezStringBuilder& sCopiedFile, ezUInt8 uiFileCopyNumber)
{
  auto sPluginName = ezStringView(szPluginName);

  sOriginalFile = ezOSFile::GetApplicationDirectory();
  sOriginalFile.AppendPath(sPluginName);
  sOriginalFile.Append(".dll");

  sCopiedFile = ezOSFile::GetApplicationDirectory();
  sCopiedFile.AppendPath(sPluginName);

  if (!ezOSFile::ExistsFile(sOriginalFile))
  {
    sOriginalFile = ezOSFile::GetCurrentWorkingDirectory();
    sOriginalFile.AppendPath(sPluginName);
    sOriginalFile.Append(".dll");

    sCopiedFile = ezOSFile::GetCurrentWorkingDirectory();
    sCopiedFile.AppendPath(sPluginName);
  }

  if (uiFileCopyNumber > 0)
    sCopiedFile.AppendFormat("{0}", uiFileCopyNumber);

  sCopiedFile.Append(".loaded");
}

ezResult UnloadPluginModule(ezPluginModule& Module, const char* szPluginFile)
{
  // reset last error code
  SetLastError(ERROR_SUCCESS);

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
  SetLastError(ERROR_SUCCESS);

#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  ezStringBuilder relativePath = szFileToLoad;
  EZ_SUCCEED_OR_RETURN(relativePath.MakeRelativeTo(ezOSFile::GetApplicationDirectory()));
  Module = LoadPackagedLibrary(ezStringWChar(relativePath).GetData(), 0);
#  else
  Module = LoadLibraryW(ezStringWChar(szFileToLoad).GetData());
#  endif

  if (Module == nullptr)
  {
    const DWORD err = GetLastError();
    ezLog::Error("Could not load plugin '{0}'. Error-Code {1}", szPluginFile, ezArgErrorCode(err));

    if (err == 126)
    {
      ezLog::Error("Please Note: This means that the plugin exists, but a DLL dependency of the plugin is missing. You probably need to copy 3rd "
                   "party DLLs next to the plugin.");
    }

    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

#else
#  error "This file should not have been included."
#endif
