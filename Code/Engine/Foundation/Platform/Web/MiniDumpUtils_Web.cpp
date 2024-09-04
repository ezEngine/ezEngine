#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WEB)

#  include <Foundation/System/MiniDumpUtils.h>

ezStatus ezMiniDumpUtils::WriteExternalProcessMiniDump(ezStringView sDumpFile, ezUInt32 uiProcessID, ezDumpType dumpTypeOverride)
{
  return ezStatus("Not implemented on Web");
}

ezStatus ezMiniDumpUtils::LaunchMiniDumpTool(ezStringView sDumpFile, ezDumpType dumpTypeOverride)
{
  return ezStatus("Not implemented on Web");
}

#endif
