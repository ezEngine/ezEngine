#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Basics/Platform/Win/MinWindows.h>
#include <Foundation/Types/Status.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
extern "C"
{
  struct _EXCEPTION_POINTERS;
}
#endif

/// \brief Functionality for writing process mini-dumps (callstacks, memory snapshots, etc)
struct EZ_FOUNDATION_DLL ezMiniDumpUtils
{
  /// \brief Tries to write a mini-dump for the external process with the given process ID.
  ///
  /// \sa WriteProcessMiniDump()
  static ezStatus WriteExternalProcessMiniDump(const char* szDumpFile, ezUInt32 uiProcessID);

  /// \brief Tries to launch ez's 'MiniDumpTool' to write a mini-dump for THIS process (the recommended way when an application is crashing).
  ///
  /// \note On Windows: If the command line option '-fullcrashdumps' is specified, it is forwarded to the MiniDumpTool.
  static ezStatus LaunchMiniDumpTool(const char* szDumpFile);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  /// \brief Windows-specific implementation for writing a mini-dump of the running process.
  ///
  /// \sa WriteProcessMiniDump()
  static ezStatus WriteOwnProcessMiniDump(const char* szDumpFile, struct _EXCEPTION_POINTERS* pExceptionInfo);

  /// \brief Given a process ID this function tries to get a HANDLE to the process with the necessary access rights to write a mini-dump.
  static ezMinWindows::HANDLE GetProcessHandleWithNecessaryRights(ezUInt32 uiProcessID);

  /// \brief Windows-specific implementation for writing a mini-dump of another process.
  ///
  /// \sa WriteProcessMiniDump()
  static ezStatus WriteExternalProcessMiniDump(const char* szDumpFile, ezUInt32 uiProcessID, ezMinWindows::HANDLE hProcess);

  /// \brief Windows-specific implementation for writing a mini-dump of the running process.
  ///
  /// \note On Windows: If the command line option '-fullcrashdumps' is specified, a crash-dump with a full memory capture is made.
  static ezStatus WriteProcessMiniDump(const char* szDumpFile, ezUInt32 uiProcessID, ezMinWindows::HANDLE hProcess, struct _EXCEPTION_POINTERS* pExceptionInfo);

#endif
};
