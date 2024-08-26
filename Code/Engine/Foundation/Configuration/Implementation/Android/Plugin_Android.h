#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Configuration/Plugin.h>

using ezPluginModule = void*;

bool ezPlugin::PlatformNeedsPluginCopy()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return false;
}

void ezPlugin::GetPluginPaths(ezStringView sPluginName, ezStringBuilder& sOriginalFile, ezStringBuilder& sCopiedFile, ezUInt8 uiFileCopyNumber)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezResult UnloadPluginModule(ezPluginModule& Module, ezStringView sPluginFile)
{
  EZ_ASSERT_NOT_IMPLEMENTED;

  return EZ_FAILURE;
}

ezResult LoadPluginModule(ezStringView sFileToLoad, ezPluginModule& Module, ezStringView sPluginFile)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}
