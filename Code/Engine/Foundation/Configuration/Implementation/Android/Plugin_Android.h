#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Configuration/Plugin.h>

typedef void* ezPluginModule;

void ezPlugin::GetPluginPaths(const char* szPluginName, ezStringBuilder& sOriginalFile, ezStringBuilder& sCopiedFile, ezUInt8 uiFileCopyNumber)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezResult UnloadPluginModule(ezPluginModule& Module, const char* szPluginFile)
{
  EZ_ASSERT_NOT_IMPLEMENTED;

  return EZ_FAILURE;
}

ezResult LoadPluginModule(const char* szFileToLoad, ezPluginModule& Module, const char* szPluginFile)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}
