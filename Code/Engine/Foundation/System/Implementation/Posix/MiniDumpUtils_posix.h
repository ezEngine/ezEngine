#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/MiniDumpUtils.h>

ezStatus ezMiniDumpUtils::WriteExternalProcessMiniDump(const char* szDumpFile, ezUInt32 uiProcessID)
{
  return ezStatus("Not implemented on Posix");
}

ezStatus ezMiniDumpUtils::LaunchMiniDumpTool(const char* szDumpFile)
{
  return ezStatus("Not implemented on Posix");
}
