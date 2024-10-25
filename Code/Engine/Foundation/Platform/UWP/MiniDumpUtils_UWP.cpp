#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

#  include <Foundation/System/MiniDumpUtils.h>

ezStatus ezMiniDumpUtils::WriteExternalProcessMiniDump(ezStringView sDumpFile, ezUInt32 uiProcessID, ezDumpType dumpTypeOverride)
{
  EZ_IGNORE_UNUSED(sDumpFile);
  EZ_IGNORE_UNUSED(uiProcessID);
  EZ_IGNORE_UNUSED(dumpTypeOverride);
  return ezStatus("Not implemented on UWP");
}

ezStatus ezMiniDumpUtils::LaunchMiniDumpTool(ezStringView sDumpFile, ezDumpType dumpTypeOverride)
{
  EZ_IGNORE_UNUSED(sDumpFile);
  EZ_IGNORE_UNUSED(dumpTypeOverride);
  return ezStatus("Not implemented on UWP");
}

#endif
