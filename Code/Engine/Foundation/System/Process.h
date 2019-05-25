#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>

typedef void* ezOsProcessHandle;
typedef ezUInt32 ezOsProcessID;

enum class ezProcessState
{
  NotStarted,
  Running,
  Finished
};

/// \brief Options that describe how to run an external process
struct EZ_FOUNDATION_DLL ezProcessOptions
{
  /// Path to the binary to launch
  ezString m_sProcess;

  /// Arguments to pass to the process. Strings that contain spaces will be wrapped in quotation marks automatically
  ezHybridArray<ezString, 8> m_Arguments;

  /// If set to true, command line tools will not show their console window, but execute in the background
  bool m_bHideConsoleWindow = true;

  /// If set, stdout will be captured and this function called on a separate thread. Requires bWaitForResult to be true.
  ezDelegate<void(ezStringView)> m_onStdOut;

  /// If set, stderr will be captured and this function called on a separate thread. Requires bWaitForResult to be true.
  ezDelegate<void(ezStringView)> m_onStdError;

  /// \brief Appends a formatted argument to m_Arguments
  ///
  /// This can be useful if a complex command needs to be added as a single argument.
  /// Ie. since arguments with spaces will be wrapped in quotes, it can make a difference
  /// whether a complex parameter is added as one or multiple arguments.
  void AddArgument(const ezFormatString& arg);

  /// \brief Overload of AddArgument(ezFormatString) for convenience.
  template <typename... ARGS>
  void AddArgument(const char* szFormat, ARGS&&... args)
  {
    AddArgument(ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Takes a full command line and appends it as individual arguments by splitting it along white-space and quotation marks.
  ///
  /// Brief, use this, if arguments are already pre-built as a full command line.
  void AddCommandLine(const char* szCmdLine);

  /// \brief Builds the command line from the process arguments and appends it to \a out_sCmdLine.
  void BuildCommandLineString(ezStringBuilder& out_sCmdLine) const;
};

/// \brief Flags for ezProcess::Launch()
struct ezProcessLaunchFlags
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    None = 0,
    Detached = EZ_BIT(0),  ///< The process will be detached right after launch, as if ezProcess::Detach() was called.
    Suspended = EZ_BIT(1), ///< The process will be launched in a suspended state. Call ezProcess::ResumeSuspended() to unpause it.
    Default = None
  };

  struct Bits
  {
    StorageType Detached : 1;
    StorageType Suspended : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezProcessLaunchFlags);

/// \brief Provides functionality to launch other processes
class EZ_FOUNDATION_DLL ezProcess
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezProcess);

public:
  ezProcess();
  ezProcess(ezProcess&& rhs);

  /// \brief Upon destruction the running process will be terminated.
  ///
  /// Use Detach() to prevent the termination of the launched process.
  ///
  /// \sa Terminate()
  /// \sa Detach()
  ~ezProcess();

  /// \brief Launches the specified process and waits for it to finish.
  static ezResult Execute(const ezProcessOptions& opt, ezInt32* out_iExitCode = nullptr);

  /// \brief Launches the specified process asynchronously.
  ///
  /// When the function returns, the process is typically starting or running.
  /// Call WaitToFinish() to wait for the process to shutdown or Terminate() to kill it.
  ///
  /// \sa ezProcessLaunchFlags
  ezResult Launch(const ezProcessOptions& opt, ezBitflags<ezProcessLaunchFlags> launchFlags = ezProcessLaunchFlags::None);

  /// \brief Resumes a process that was launched in a suspended state. Returns EZ_FAILURE if the process has not been launched or already
  /// resumed.
  ezResult ResumeSuspended();

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

  /// \brief Returns the OS specific handle to the process
  ezOsProcessHandle GetProcessHandle() const;

  /// \brief Returns the OS-specific process ID (PID)
  ezOsProcessID GetProcessID() const;

  /// \brief Returns OS-specific process ID (PID) for the calling process
  static ezOsProcessID GetCurrentProcessID();

private:
  void BuildFullCommandLineString(const ezProcessOptions& opt, const char* szProcess, ezStringBuilder& cmd) const;

  ezUniquePtr<struct ezProcessImpl> m_impl;

  // the default value is used by GetExitCode() to determine whether it has to be reevaluated
  mutable ezInt32 m_iExitCode = -0xFFFF;

  ezString m_sProcess;
  ezDelegate<void(ezStringView)> m_onStdOut;
  ezDelegate<void(ezStringView)> m_onStdError;
};
