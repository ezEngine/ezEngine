#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_OSX)

#  include <Foundation/System/MiniDumpUtils.h>

ezStatus ezMiniDumpUtils::WriteExternalProcessMiniDump(ezStringView sDumpFile, ezUInt32 uiProcessID)
{
  return ezStatus("Not implemented on OSX");
}

ezStatus ezMiniDumpUtils::LaunchMiniDumpTool(ezStringView sDumpFile)
{
  return ezStatus("Not implemented on OSX");
}

#endif
