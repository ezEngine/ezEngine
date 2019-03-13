#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/UniquePtr.h>

typedef void* ezOsProcessHandle;
typedef ezUInt32 ezOsProcessID;

enum class ezProcessState
{
  NotStarted,
  Running,
  Finished
};

/// \brief Provides functionality to launch other processes
class EZ_FOUNDATION_DLL ezProcess
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezProcess);

public:
  /// Path to the binary to launch
  ezString m_sProcess;

  /// Arguments to pass to the process. Strings that contain spaces will be wrapped in quotation marks automatically
  ezHybridArray<ezString, 8> m_Arguments;

  /// If set to true, command line tools will not show their console window, but execute in the background
  bool m_bHideConsoleWindow = true;

  /// If set, stdout will be captured and this function called on a separate thread. Requires bWaitForResult to be true.
  ezDelegate<void(ezStringView)> m_onStdOut;

  /// If set, stderror will be captured and this function called on a separate thread. Requires bWaitForResult to be true.
  ezDelegate<void(ezStringView)> m_onStdError;

  ezProcess();

  /// \brief Upon destruction the running process will be terminated.
  ///
  /// Use Detach() to prevent the termination of the launched process.
  ///
  /// \sa Terminate()
  /// \sa Detach()
  ~ezProcess();

  /// \brief Appends a formatted argument to m_Arguments
  ///
  /// This can be useful if a complex command needs to be added as a single argument.
  /// Ie. since arguments with spaces will be wrapped in quotes, it can make a difference
  /// whether a complex parameter is added as one or multiple arguments.
  void AddArgument(const ezFormatString& arg);

  /// \brief Launches the specified process and waits for it to finish.
  ezResult Launch();

  /// \brief Launches the specified process in a detached state.
  ///
  /// When the function returns, the process is typically starting or running.
  /// Call WaitToFinish() to wait for the process to shutdown or Terminate() to kill it.
  ezResult LaunchAsync();

  /// \brief Waits the given amount of time for the previously launched process to finish.
  ///
  /// Pass in ezTime::Zero() to wait indefinitely.
  /// Returns EZ_FAILURE, if the process did not finish within the given time.
  ///
  /// \note Asserts that the ezProcess instance was used to successfully launch a process before.
  ezResult WaitToFinish(ezTime timeout = ezTime::Zero());

  /// \brief Kills the detached process, if possible.
  ezResult Terminate();

  /// \brief Returns the exit code of the process. The exit code will be -0xFFFF as long as the process has not finished.
  ezInt32 GetExitCode() const;

  /// \brief Returns the running state of the process
  ///
  /// If the state is 'finished' the exit code (as returned by GetExitCode() ) will be updated.
  ezProcessState GetState() const;

  /// \brief Detaches the running process from the ezProcess instance.
  ///
  /// This means the ezProcess instance loses control over terminating the process or communicating with it.
  /// It also means that the process will keep running and not get terminated when the ezProcess instance is destroyed.
  void Detach();

private:
  void BuildCommandLineString(const char* szProcess, ezStringBuilder& cmd) const;

  ezUniquePtr<struct ezProcessImpl> m_impl;

  // the default value is used by GetExitCode() to determine whether it has to be reevaluated
  mutable ezInt32 m_iExitCode = -0xFFFF;
};
