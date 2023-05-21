#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <dlfcn.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>

using ezPluginModule = void*;

void ezPlugin::GetPluginPaths(const char* szPluginName, ezStringBuilder& sOriginalFile, ezStringBuilder& sCopiedFile, ezUInt8 uiFileCopyNumber)
{
  sOriginalFile = ezOSFile::GetApplicationDirectory();
  sOriginalFile.AppendPath(szPluginName);
  sOriginalFile.Append(".so");

  sCopiedFile = ezOSFile::GetApplicationDirectory();
  sCopiedFile.AppendPath(szPluginName);

  if (uiFileCopyNumber > 0)
    sCopiedFile.AppendFormat("{0}", uiFileCopyNumber);

  sCopiedFile.Append(".loaded");
}

ezResult UnloadPluginModule(ezPluginModule& Module, const char* szPluginFile)
{
  if (dlclose(Module) != 0)
  {
    ezLog::Error("Could not unload plugin '{0}'. Error {1}", szPluginFile, static_cast<const char*>(dlerror()));
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult LoadPluginModule(const char* szFileToLoad, ezPluginModule& Module, const char* szPluginFile)
{
  Module = dlopen(szFileToLoad, RTLD_NOW | RTLD_GLOBAL);
  if (Module == nullptr)
  {
    ezLog::Error("Could not load plugin '{0}'. Error {1}.\nSet the environment variable LD_DEBUG=all to get more information.", szPluginFile, static_cast<const char*>(dlerror()));
    return EZ_FAILURE;
  }
  return EZ_SUCCESS;
}
