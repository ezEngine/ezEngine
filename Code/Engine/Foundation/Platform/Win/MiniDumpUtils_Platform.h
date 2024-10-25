#pragma once

#include <Foundation/Platform/Win/Utils/MinWindows.h>

extern "C"
{
  struct _EXCEPTION_POINTERS;
}

namespace ezMiniDumpUtils
{
  /// \brief Windows-specific implementation for writing a mini-dump of the running process.
  ///
  /// \sa WriteProcessMiniDump()
  ezStatus EZ_FOUNDATION_DLL WriteOwnProcessMiniDump(ezStringView sDumpFile, struct _EXCEPTION_POINTERS* pExceptionInfo, ezDumpType dumpTypeOverride = ezDumpType::Auto);

  /// \brief Given a process ID this function tries to get a HANDLE to the process with the necessary access rights to write a mini-dump.
  ezMinWindows::HANDLE EZ_FOUNDATION_DLL GetProcessHandleWithNecessaryRights(ezUInt32 uiProcessID);

  /// \brief Windows-specific implementation for writing a mini-dump of another process.
  ///
  /// \sa WriteProcessMiniDump()
  ezStatus EZ_FOUNDATION_DLL WriteExternalProcessMiniDump(ezStringView sDumpFile, ezUInt32 uiProcessID, ezMinWindows::HANDLE hProcess, ezDumpType dumpTypeOverride = ezDumpType::Auto);

  /// \brief Windows-specific implementation for writing a mini-dump of the running process.
  ///
  /// \note On Windows: A crash-dump with a full memory capture is made if either this application's command line option '-fullcrashdumps' is specified or if that setting is overridden through dumpTypeOverride = ezDumpType::MiniDumpWithFullMemory.
  ezStatus EZ_FOUNDATION_DLL WriteProcessMiniDump(ezStringView sDumpFile, ezUInt32 uiProcessID, ezMinWindows::HANDLE hProcess, struct _EXCEPTION_POINTERS* pExceptionInfo, ezDumpType dumpTypeOverrideType = ezDumpType::Auto);

}; // namespace ezMiniDumpUtils
