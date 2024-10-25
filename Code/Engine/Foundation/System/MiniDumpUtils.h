#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Types/Status.h>

/// \brief Functionality for writing process mini-dumps (callstacks, memory snapshots, etc)
namespace ezMiniDumpUtils
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
  ezStatus EZ_FOUNDATION_DLL WriteExternalProcessMiniDump(ezStringView sDumpFile, ezUInt32 uiProcessID, ezDumpType dumpTypeOverride = ezDumpType::Auto);

  /// \brief Tries to launch ez's 'MiniDumpTool' to write a mini-dump for THIS process (the recommended way when an application is crashing).
  ///
  /// \note On Windows: The command line option '-fullcrashdumps' is passed if either set in this application's command line or if overridden through dumpTypeOverride = ezDumpType::MiniDumpWithFullMemory.
  ezStatus EZ_FOUNDATION_DLL LaunchMiniDumpTool(ezStringView sDumpFile, ezDumpType dumpTypeOverride = ezDumpType::Auto);

}; // namespace ezMiniDumpUtils


#if EZ_ENABLED(EZ_SUPPORTS_CRASH_DUMPS)

#  include <MiniDumpUtils_Platform.h>

#endif
