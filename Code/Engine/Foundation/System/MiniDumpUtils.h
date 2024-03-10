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

  /// \brief Specifies the dump mode that is written.
  enum class ezDumpType
  {
    Auto,                  ///< Uses the setting specified globally through the command line.
    MiniDump,              ///< Saves a mini-dump without full memory, regardless of this application's command line flag '-fullcrashdumps'.
    MiniDumpWithFullMemory ///< Saves a mini-dump with full memory, regardless of this application's command line flag '-fullcrashdumps'.
  };

  /// \brief Tries to write a mini-dump for the external process with the given process ID.
  ///
  /// \sa WriteProcessMiniDump()
  static ezStatus WriteExternalProcessMiniDump(ezStringView sDumpFile, ezUInt32 uiProcessID, ezDumpType dumpTypeOverride = ezDumpType::Auto);

  /// \brief Tries to launch ez's 'MiniDumpTool' to write a mini-dump for THIS process (the recommended way when an application is crashing).
  ///
  /// \note On Windows: The command line option '-fullcrashdumps' is passed if either set in this application's command line or if overridden through dumpTypeOverride = ezDumpType::MiniDumpWithFullMemory.
  static ezStatus LaunchMiniDumpTool(ezStringView sDumpFile, ezDumpType dumpTypeOverride = ezDumpType::Auto);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  /// \brief Windows-specific implementation for writing a mini-dump of the running process.
  ///
  /// \sa WriteProcessMiniDump()
  static ezStatus WriteOwnProcessMiniDump(ezStringView sDumpFile, struct _EXCEPTION_POINTERS* pExceptionInfo, ezDumpType dumpTypeOverride = ezDumpType::Auto);

  /// \brief Given a process ID this function tries to get a HANDLE to the process with the necessary access rights to write a mini-dump.
  static ezMinWindows::HANDLE GetProcessHandleWithNecessaryRights(ezUInt32 uiProcessID);

  /// \brief Windows-specific implementation for writing a mini-dump of another process.
  ///
  /// \sa WriteProcessMiniDump()
  static ezStatus WriteExternalProcessMiniDump(ezStringView sDumpFile, ezUInt32 uiProcessID, ezMinWindows::HANDLE hProcess, ezDumpType dumpTypeOverride = ezDumpType::Auto);

  /// \brief Windows-specific implementation for writing a mini-dump of the running process.
  ///
  /// \note On Windows: A crash-dump with a full memory capture is made if either this application's command line option '-fullcrashdumps' is specified or if that setting is overridden through dumpTypeOverride = ezDumpType::MiniDumpWithFullMemory.
  static ezStatus WriteProcessMiniDump(ezStringView sDumpFile, ezUInt32 uiProcessID, ezMinWindows::HANDLE hProcess, struct _EXCEPTION_POINTERS* pExceptionInfo, ezDumpType dumpTypeOverrideType = ezDumpType::Auto);

#endif
};
