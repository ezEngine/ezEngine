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
  /// \brief Checks if the MiniDumpUtils are supported on the current platform
  static bool IsSupported();

  /// \brief Tries to write a mini-dump for the external process with the given process ID.
  static ezStatus WriteExternalProcessMiniDump(const char* szDumpFile, ezUInt32 uiProcessID);

  /// \brief Tries to launch ez's 'MiniDumpTool' to write a mini-dump for THIS process (the recommended way when an application is crashing).
  static ezStatus LaunchMiniDumpTool(const char* szDumpFile);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  /// \brief Windows-specific implementation for writing a mini-dump of the running process.
  static ezStatus WriteOwnProcessMiniDump(const char* szDumpFile, struct _EXCEPTION_POINTERS* pExceptionInfo);

  /// \brief Given a process ID this function tries to get a HANDLE to the process with the necessary access rights to write a mini-dump.
  static ezMinWindows::HANDLE GetProcessHandleWithNecessaryRights(ezUInt32 uiProcessID);

  /// \brief Windows-specific implementation for writing a mini-dump of another process.
  static ezStatus WriteExternalProcessMiniDump(const char* szDumpFile, ezUInt32 uiProcessID, ezMinWindows::HANDLE hProcess);

  /// \brief Windows-specific implementation for writing a mini-dump of the running process.
  static ezStatus WriteProcessMiniDump(const char* szDumpFile, ezUInt32 uiProcessID, ezMinWindows::HANDLE hProcess, struct _EXCEPTION_POINTERS* pExceptionInfo);

#endif
};
