#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/MiniDumpUtils.h>

ezStatus ezMiniDumpUtils::WriteExternalProcessMiniDump(ezStringView sDumpFile, ezUInt32 uiProcessID)
{
  return ezStatus("Not implemented on Posix");
}

ezStatus ezMiniDumpUtils::LaunchMiniDumpTool(ezStringView sDumpFile)
{
  return ezStatus("Not implemented on Posix");
}
