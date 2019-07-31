#pragma once

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
#include <Foundation/System/Process.h>

/// \brief Process groups are used to tie multiple processes together and ensure they get terminated either on demand or when the
/// application crashes
///
/// When an ezProcessGroup instance is destroyed (either normally or due to a crash), all processes that have
/// been added to the group will be terminated by the OS.
///
/// Only processes that were launched asynchronously and in a suspended state can be added to process groups.
/// They will be resumed by the group.
class EZ_FOUNDATION_DLL ezProcessGroup
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezProcessGroup);

public:
  /// \brief Creates a process group. The name is only used for debugging purposes.
  ezProcessGroup(const char* szGroupName = nullptr);
  ~ezProcessGroup();

  /// \brief Launches a new process in the group.
  ezResult Launch(const ezProcessOptions& opt);

  /// \brief Waits for all the processes in the group to terminate.
  ///
  /// Returns EZ_SUCCESS only if all processes have shut down.
  /// In all other cases, e.g. if the optional timeout is reached,
  /// EZ_FAILURE is returned.
  ezResult WaitToFinish(ezTime timeout = ezTime::Zero());

  /// \brief Tries to kill all processes associated with this group.
  ///
  /// Sends a kill command to all processes and then waits indefinitely for them to terminate.
  ezResult TerminateAll(ezInt32 iForcedExitCode = -2);

  /// \brief Returns the container holding all processes of this group.
  ///
  /// This can be used to query per-process information such as exit codes.
  const ezHybridArray<ezProcess, 8>& GetProcesses() const;

private:
  ezUniquePtr<struct ezProcessGroupImpl> m_impl;

  ezHybridArray<ezProcess, 8> m_Processes;
};
#endif
