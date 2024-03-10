#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <dlfcn.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/Process.h>

using ezPluginModule = void*;

bool ezPlugin::PlatformNeedsPluginCopy()
{
  return false;
}

void ezPlugin::GetPluginPaths(ezStringView sPluginName, ezStringBuilder& sOriginalFile, ezStringBuilder& sCopiedFile, ezUInt8 uiFileCopyNumber)
{
  sOriginalFile = ezOSFile::GetApplicationDirectory();
  sOriginalFile.AppendPath(sPluginName);
  sOriginalFile.Append(".so");

  sCopiedFile = ezOSFile::GetApplicationDirectory();
  sCopiedFile.AppendPath(sPluginName);

  if (uiFileCopyNumber > 0)
    sCopiedFile.AppendFormat("{0}", uiFileCopyNumber);

  sCopiedFile.Append(".loaded");
}

ezResult UnloadPluginModule(ezPluginModule& Module, ezStringView sPluginFile)
{
  if (dlclose(Module) != 0)
  {
    ezStringBuilder tmp;
    ezLog::Error("Could not unload plugin '{0}'. Error {1}", sPluginFile.GetData(tmp), static_cast<const char*>(dlerror()));
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult LoadPluginModule(ezStringView sFileToLoad, ezPluginModule& Module, ezStringView sPluginFile)
{
  ezStringBuilder tmp;
  Module = dlopen(sFileToLoad.GetData(tmp), RTLD_NOW | RTLD_GLOBAL);
  if (Module == nullptr)
  {
    ezLog::Error("Could not load plugin '{0}'. Error {1}.\nSet the environment variable LD_DEBUG=all to get more information.", sPluginFile.GetData(tmp), static_cast<const char*>(dlerror()));
    return EZ_FAILURE;
  }
  return EZ_SUCCESS;
}
