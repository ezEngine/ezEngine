#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/System/Process.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <Foundation/System/SystemInformation.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef _EZ_DEFINED_POLLFD_POD
#  define _EZ_DEFINED_POLLFD_POD
EZ_DEFINE_AS_POD_TYPE(struct pollfd);
#endif

class ezFd
{
public:
  ezFd() = default;
  ezFd(const ezFd&) = delete;
  ezFd(ezFd&& other)
  {
    m_fd = other.m_fd;
    other.m_fd = -1;
  }

  ~ezFd()
  {
    Close();
  }

  void Close()
  {
    if (m_fd != -1)
    {
      close(m_fd);
      m_fd = -1;
    }
  }

  bool IsValid() const
  {
    return m_fd >= 0;
  }

  void operator=(const ezFd&) = delete;
  void operator=(ezFd&& other)
  {
    Close();
    m_fd = other.m_fd;
    other.m_fd = -1;
  }

  void TakeOwnership(int fd)
  {
    Close();
    m_fd = fd;
  }

  int Borrow() const { return m_fd; }

  int Detach()
  {
    auto result = m_fd;
    m_fd = -1;
    return result;
  }

  ezResult AddFlags(int addFlags)
  {
    if (m_fd < 0)
      return EZ_FAILURE;

    if (addFlags & O_CLOEXEC)
    {
      int flags = fcntl(m_fd, F_GETFD);
      flags |= FD_CLOEXEC;
      if (fcntl(m_fd, F_SETFD, flags) != 0)
      {
        ezLog::Error("Failed to set flags on {}: {}", m_fd, errno);
        return EZ_FAILURE;
      }
      addFlags &= ~O_CLOEXEC;
    }

    if (addFlags)
    {
      int flags = fcntl(m_fd, F_GETFL);
      flags |= addFlags;
      if (fcntl(m_fd, F_SETFD, flags) != 0)
      {
        ezLog::Error("Failed to set flags on {}: {}", m_fd, errno);
        return EZ_FAILURE;
      }
    }

    return EZ_SUCCESS;
  }

  static ezResult MakePipe(ezFd (&fds)[2], int flags = 0)
  {
    fds[0].Close();
    fds[1].Close();
#if EZ_ENABLED(EZ_USE_LINUX_POSIX_EXTENSIONS)
    if (pipe2((int*)fds, flags) != 0)
    {
      return EZ_FAILURE;
    }
#else
    if (pipe((int*)fds) != 0)
    {
      return EZ_FAILURE;
    }
    if (flags != 0 && (fds[0].AddFlags(flags).Failed() || fds[1].AddFlags(flags).Failed()))
    {
      fds[0].Close();
      fds[1].Close();
      return EZ_FAILURE;
    }
#endif
    return EZ_SUCCESS;
  }

private:
  int m_fd = -1;
};

namespace
{
  struct ProcessStartupError
  {
    enum class Type : ezUInt32
    {
      FailedToChangeWorkingDirectory = 0,
      FailedToExecv = 1
    };

    Type type;
    int errorCode;
  };
} // namespace


struct ezProcessImpl
{
  ~ezProcessImpl()
  {
    StopStreamWatcher();
  }

  pid_t m_childPid = -1;
  bool m_exitCodeAvailable = false;
  bool m_processSuspended = false;

  struct StdStreamInfo
  {
    ezFd fd;
    ezDelegate<void(ezStringView)> callback;
  };
  ezHybridArray<StdStreamInfo, 2> m_streams;
  ezDynamicArray<ezStringBuilder> m_overflowBuffers;
  ezUniquePtr<ezOSThread> m_streamWatcherThread;
  ezFd m_wakeupPipeReadEnd;
  ezFd m_wakeupPipeWriteEnd;

  static void* StreamWatcherThread(void* context)
  {
    ezProcessImpl* self = reinterpret_cast<ezProcessImpl*>(context);
    char buffer[4096];

    ezHybridArray<struct pollfd, 3> pollfds;

    pollfds.PushBack({self->m_wakeupPipeReadEnd.Borrow(), POLLIN, 0});
    for (StdStreamInfo& stream : self->m_streams)
    {
      pollfds.PushBack({stream.fd.Borrow(), POLLIN, 0});
    }

    bool run = true;
    while (run)
    {
      int result = poll(pollfds.GetData(), pollfds.GetCount(), -1);
      if (result > 0)
      {
        // Result at index 0 is special and means there was a WakeUp
        if (pollfds[0].revents != 0)
        {
          run = false;
        }

        for (ezUInt32 i = 1; i < pollfds.GetCount(); ++i)
        {
          if (pollfds[i].revents & POLLIN)
          {
            ezStringBuilder& overflowBuffer = self->m_overflowBuffers[i - 1];
            StdStreamInfo& stream = self->m_streams[i - 1];
            while (true)
            {
              ssize_t numBytes = read(stream.fd.Borrow(), buffer, EZ_ARRAY_SIZE(buffer));
              if (numBytes < 0)
              {
                if (errno == EWOULDBLOCK)
                {
                  break;
                }
                ezLog::Error("Process Posix read error on {}: {}", stream.fd.Borrow(), errno);
                return nullptr;
              }

              const char* szCurrentPos = buffer;
              const char* szEndPos = buffer + numBytes;
              while (szCurrentPos < szEndPos)
              {
                const char* szFound = ezStringUtils::FindSubString(szCurrentPos, "\n", szEndPos);
                if (szFound)
                {
                  if (overflowBuffer.IsEmpty())
                  {
                    // If there is nothing in the overflow buffer this is a complete line and can be fired as is.
                    stream.callback(ezStringView(szCurrentPos, szFound + 1));
                  }
                  else
                  {
                    // We have data in the overflow buffer so this is the final part of a partial line so we need to complete and fire the overflow buffer.
                    overflowBuffer.Append(ezStringView(szCurrentPos, szFound + 1));
                    stream.callback(overflowBuffer);
                    overflowBuffer.Clear();
                  }
                  szCurrentPos = szFound + 1;
                }
                else
                {
                  // This is either the start or a middle segment of a line, append to overflow buffer.
                  overflowBuffer.Append(ezStringView(szCurrentPos, szEndPos));
                  szCurrentPos = szEndPos;
                }
              }

              if (numBytes < EZ_ARRAY_SIZE(buffer))
              {
                break;
              }
            }
          }
          pollfds[i].revents = 0;
        }
      }
      else if (result < 0)
      {
        ezLog::Error("poll error {}", errno);
        break;
      }
    }

    for (ezUInt32 i = 0; i < self->m_streams.GetCount(); ++i)
    {
      ezStringBuilder& overflowBuffer = self->m_overflowBuffers[i];
      if (!overflowBuffer.IsEmpty())
      {
        self->m_streams[i].callback(overflowBuffer);
        overflowBuffer.Clear();
      }

      self->m_streams[i].fd.Close();
    }

    return nullptr;
  }

  ezResult StartStreamWatcher()
  {
    ezFd wakeupPipe[2];
    if (ezFd::MakePipe(wakeupPipe, O_NONBLOCK | O_CLOEXEC).Failed())
    {
      ezLog::Error("Failed to setup wakeup pipe {}", errno);
      return EZ_FAILURE;
    }
    else
    {
      m_wakeupPipeReadEnd = std::move(wakeupPipe[0]);
      m_wakeupPipeWriteEnd = std::move(wakeupPipe[1]);
    }

    m_streamWatcherThread = EZ_DEFAULT_NEW(ezOSThread, &StreamWatcherThread, this, "StdStrmWtch");
    m_streamWatcherThread->Start();

    return EZ_SUCCESS;
  }

  void StopStreamWatcher()
  {
    if (m_streamWatcherThread)
    {
      char c = 0;
      EZ_IGNORE_UNUSED(write(m_wakeupPipeWriteEnd.Borrow(), &c, 1));
      m_streamWatcherThread->Join();
      m_streamWatcherThread = nullptr;
    }
    m_wakeupPipeReadEnd.Close();
    m_wakeupPipeWriteEnd.Close();
  }

  void AddStream(ezFd fd, const ezDelegate<void(ezStringView)>& callback)
  {
    m_streams.PushBack({std::move(fd), callback});
    m_overflowBuffers.SetCount(m_streams.GetCount());
  }

  ezUInt32 GetNumStreams() const { return m_streams.GetCount(); }

  static ezResult StartChildProcess(const ezProcessOptions& opt, pid_t& outPid, bool suspended, ezFd& outStdOutFd, ezFd& outStdErrFd)
  {
    ezFd stdoutPipe[2];
    ezFd stderrPipe[2];
    ezFd startupErrorPipe[2];

    ezStringBuilder executablePath = opt.m_sProcess;
    ezFileStats stats;
    if (!opt.m_sProcess.IsAbsolutePath())
    {
      executablePath = ezOSFile::GetCurrentWorkingDirectory();
      executablePath.AppendPath(opt.m_sProcess);
    }

    if (ezOSFile::GetFileStats(executablePath, stats).Failed() || stats.m_bIsDirectory)
    {
      ezHybridArray<char, 512> confPath;
      auto envPATH = getenv("PATH");
      if (envPATH == nullptr) // if no PATH environment variable is available, we need to fetch the system default;
      {
#if _POSIX_C_SOURCE >= 2 || _XOPEN_SOURCE
        size_t confPathSize = confstr(_CS_PATH, nullptr, 0);
        if (confPathSize > 0)
        {
          confPath.SetCountUninitialized(confPathSize);
          if (confstr(_CS_PATH, confPath.GetData(), confPath.GetCount()) == 0)
          {
            confPath.SetCountUninitialized(0);
          }
        }
#endif
        if (confPath.GetCount() == 0)
        {
          confPath.PushBack('\0');
        }
        envPATH = confPath.GetData();
      }

      ezStringView path = envPATH;
      ezHybridArray<ezStringView, 16> pathParts;
      path.Split(false, pathParts, ":");

      for (auto& pathPart : pathParts)
      {
        executablePath = pathPart;
        executablePath.AppendPath(opt.m_sProcess);
        if (ezOSFile::GetFileStats(executablePath, stats).Succeeded() && !stats.m_bIsDirectory)
        {
          break;
        }
        executablePath.Clear();
      }
    }

    if (executablePath.IsEmpty())
    {
      return EZ_FAILURE;
    }

    if (opt.m_onStdOut.IsValid())
    {
      if (ezFd::MakePipe(stdoutPipe).Failed())
      {
        return EZ_FAILURE;
      }
      if (stdoutPipe[0].AddFlags(O_NONBLOCK).Failed())
      {
        return EZ_FAILURE;
      }
    }

    if (opt.m_onStdError.IsValid())
    {
      if (ezFd::MakePipe(stderrPipe).Failed())
      {
        return EZ_FAILURE;
      }
      if (stderrPipe[0].AddFlags(O_NONBLOCK).Failed())
      {
        return EZ_FAILURE;
      }
    }

    if (ezFd::MakePipe(startupErrorPipe, O_CLOEXEC).Failed())
    {
      return EZ_FAILURE;
    }

    pid_t childPid = fork();
    if (childPid < 0)
    {
      return EZ_FAILURE;
    }

    if (childPid == 0) // We are the child
    {
      if (suspended)
      {
        if (raise(SIGSTOP) < 0)
        {
          _exit(-1);
        }
      }

      if (opt.m_bHideConsoleWindow == true)
      {
        // Redirect STDIN to /dev/null
        int stdinReplace = open("/dev/null", O_RDONLY);
        dup2(stdinReplace, STDIN_FILENO);
        close(stdinReplace);

        if (!opt.m_onStdOut.IsValid())
        {
          int stdoutReplace = open("/dev/null", O_WRONLY);
          dup2(stdoutReplace, STDOUT_FILENO);
          close(stdoutReplace);
        }

        if (!opt.m_onStdError.IsValid())
        {
          int stderrReplace = open("/dev/null", O_WRONLY);
          dup2(stderrReplace, STDERR_FILENO);
          close(stderrReplace);
        }
      }
      else
      {
        // TODO: Launch a x-terminal-emulator with the command and somehow redirect STDOUT, etc?
        EZ_ASSERT_NOT_IMPLEMENTED;
      }

      if (opt.m_onStdOut.IsValid())
      {
        stdoutPipe[0].Close();                       // We don't need the read end of the pipe in the child process
        dup2(stdoutPipe[1].Borrow(), STDOUT_FILENO); // redirect the write end to STDOUT
        stdoutPipe[1].Close();
      }

      if (opt.m_onStdError.IsValid())
      {
        stderrPipe[0].Close();                       // We don't need the read end of the pipe in the child process
        dup2(stderrPipe[1].Borrow(), STDERR_FILENO); // redirect the write end to STDERR
        stderrPipe[1].Close();
      }

      startupErrorPipe[0].Close(); // we don't need the read end of the startup error pipe in the child process

      ezHybridArray<char*, 9> args;

      args.PushBack(const_cast<char*>(executablePath.GetData()));
      for (const ezString& arg : opt.m_Arguments)
      {
        args.PushBack(const_cast<char*>(arg.GetData()));
      }
      args.PushBack(nullptr);

      if (!opt.m_sWorkingDirectory.IsEmpty())
      {
        if (chdir(opt.m_sWorkingDirectory.GetData()) < 0)
        {
          auto err = ProcessStartupError{ProcessStartupError::Type::FailedToChangeWorkingDirectory, 0};
          EZ_IGNORE_UNUSED(write(startupErrorPipe[1].Borrow(), &err, sizeof(err)));
          startupErrorPipe[1].Close();
          _exit(-1);
        }
      }

      if (execv(executablePath, args.GetData()) < 0)
      {
        auto err = ProcessStartupError{ProcessStartupError::Type::FailedToExecv, errno};
        EZ_IGNORE_UNUSED(write(startupErrorPipe[1].Borrow(), &err, sizeof(err)));
        startupErrorPipe[1].Close();
        _exit(-1);
      }
    }
    else
    {
      startupErrorPipe[1].Close(); // We don't need the write end of the startup error pipe in the parent process
      stdoutPipe[1].Close();       // Don't need the write end in the parent process
      stderrPipe[1].Close();       // Don't need the write end in the parent process

      ProcessStartupError err = {};
      auto errSize = read(startupErrorPipe[0].Borrow(), &err, sizeof(err));
      startupErrorPipe[0].Close(); // we no longer need the read end of the startup error pipe

      // There are two possible cases here
      // Case 1: errSize is equal to 0, which means no error happened on the startupErrorPipe was closed during the execv call
      // Case 2: errSize > 0 in which case there was an error before the pipe was closed normally.
      if (errSize > 0)
      {
        EZ_ASSERT_DEV(errSize == sizeof(err), "Child process should have written a full ProcessStartupError struct");
        switch (err.type)
        {
          case ProcessStartupError::Type::FailedToChangeWorkingDirectory:
            ezLog::Error("Failed to start process '{}' because the given working directory '{}' is invalid", opt.m_sProcess, opt.m_sWorkingDirectory);
            break;
          case ProcessStartupError::Type::FailedToExecv:
            ezLog::Error("Failed to exec when starting process '{}' the error code is '{}'", opt.m_sProcess, err.errorCode);
            break;
        }
        return EZ_FAILURE;
      }

      outPid = childPid;

      if (opt.m_onStdOut.IsValid())
      {
        outStdOutFd = std::move(stdoutPipe[0]);
      }

      if (opt.m_onStdError.IsValid())
      {
        outStdErrFd = std::move(stderrPipe[0]);
      }
    }

    return EZ_SUCCESS;
  }
};

ezProcess::ezProcess()
{
  m_pImpl = EZ_DEFAULT_NEW(ezProcessImpl);
}

ezProcess::~ezProcess()
{
  if (GetState() == ezProcessState::Running)
  {
    ezLog::Dev("Process still running - terminating '{}'", m_sProcess);

    Terminate().IgnoreResult();
  }

  // Explicitly clear the implementation here so that member
  // state (e.g. delegates) used by the impl survives the implementation.
  m_pImpl.Clear();
}

ezResult ezProcess::Execute(const ezProcessOptions& opt, ezInt32* out_iExitCode /*= nullptr*/)
{
  pid_t childPid = 0;
  ezFd stdoutFd;
  ezFd stderrFd;
  if (ezProcessImpl::StartChildProcess(opt, childPid, false, stdoutFd, stderrFd).Failed())
  {
    return EZ_FAILURE;
  }

  ezProcessImpl impl;
  if (stdoutFd.IsValid())
  {
    impl.AddStream(std::move(stdoutFd), opt.m_onStdOut);
  }

  if (stderrFd.IsValid())
  {
    impl.AddStream(std::move(stderrFd), opt.m_onStdError);
  }

  if (impl.GetNumStreams() > 0 && impl.StartStreamWatcher().Failed())
  {
    return EZ_FAILURE;
  }

  int childStatus = -1;
  pid_t waitedPid = waitpid(childPid, &childStatus, 0);
  if (waitedPid < 0)
  {
    return EZ_FAILURE;
  }
  if (out_iExitCode != nullptr)
  {
    if (WIFEXITED(childStatus))
    {
      *out_iExitCode = WEXITSTATUS(childStatus);
    }
    else if (WIFSIGNALED(childStatus))
    {
      *out_iExitCode = WTERMSIG(childStatus);
    }
    else
    {
      *out_iExitCode = -1;
    }
  }
  return EZ_SUCCESS;
}

ezResult ezProcess::Launch(const ezProcessOptions& opt, ezBitflags<ezProcessLaunchFlags> launchFlags /*= ezProcessLaunchFlags::None*/)
{
  EZ_ASSERT_DEV(m_pImpl->m_childPid == -1, "Can not reuse an instance of ezProcess");

  ezFd stdoutFd;
  ezFd stderrFd;

  if (ezProcessImpl::StartChildProcess(opt, m_pImpl->m_childPid, launchFlags.IsSet(ezProcessLaunchFlags::Suspended), stdoutFd, stderrFd).Failed())
  {
    return EZ_FAILURE;
  }

  m_pImpl->m_exitCodeAvailable = false;
  m_pImpl->m_processSuspended = launchFlags.IsSet(ezProcessLaunchFlags::Suspended);

  if (stdoutFd.IsValid())
  {
    m_pImpl->AddStream(std::move(stdoutFd), opt.m_onStdOut);
  }

  if (stderrFd.IsValid())
  {
    m_pImpl->AddStream(std::move(stderrFd), opt.m_onStdError);
  }

  if (m_pImpl->GetNumStreams() > 0)
  {
    if (m_pImpl->StartStreamWatcher().Failed())
    {
      return EZ_FAILURE;
    }
  }

  if (launchFlags.IsSet(ezProcessLaunchFlags::Detached))
  {
    Detach();
  }

  return EZ_SUCCESS;
}

ezResult ezProcess::ResumeSuspended()
{
  if (m_pImpl->m_childPid < 0 || !m_pImpl->m_processSuspended)
  {
    return EZ_FAILURE;
  }

  if (kill(m_pImpl->m_childPid, SIGCONT) < 0)
  {
    return EZ_FAILURE;
  }
  m_pImpl->m_processSuspended = false;
  return EZ_SUCCESS;
}

ezResult ezProcess::WaitToFinish(ezTime timeout /*= ezTime::MakeZero()*/)
{
  if (m_pImpl->m_exitCodeAvailable)
  {
    return EZ_SUCCESS;
  }

  if (timeout.IsZero())
  {
    int childStatus = 0;
    int waitResult = waitpid(m_pImpl->m_childPid, &childStatus, 0);
    if (waitResult > 0)
    {
      m_iExitCode = WEXITSTATUS(childStatus);
      m_pImpl->m_exitCodeAvailable = true;

      m_pImpl->StopStreamWatcher();

      return EZ_SUCCESS;
    }
    return EZ_FAILURE;
  }
  else
  {
    ezTime startWait = ezTime::Now();
    while (true)
    {
      const ezProcessState state = GetState();
      switch (state)
      {
        case ezProcessState::NotStarted:
          return EZ_FAILURE;
        case ezProcessState::Running:
          break;
        case ezProcessState::Finished:
          return EZ_SUCCESS;
      }

      ezTime timeSpent = ezTime::Now() - startWait;
      if (timeSpent > timeout)
      {
        return EZ_FAILURE;
      }
      ezThreadUtils::Sleep(ezMath::Min(ezTime::MakeFromMilliseconds(100.0), timeout - timeSpent));
    }
  }
  return EZ_SUCCESS;
}

ezResult ezProcess::Terminate()
{
  if (m_pImpl->m_childPid == -1)
  {
    return EZ_FAILURE;
  }

  EZ_SCOPE_EXIT(m_pImpl->StopStreamWatcher());

  if (kill(m_pImpl->m_childPid, SIGKILL) < 0)
  {
    if (errno != ESRCH) // ESRCH = Process does not exist
    {
      return EZ_FAILURE;
    }
  }
  m_pImpl->m_exitCodeAvailable = true;
  m_iExitCode = -1;

  return EZ_SUCCESS;
}

ezProcessState ezProcess::GetState() const
{
  if (m_pImpl->m_childPid == -1)
  {
    return ezProcessState::NotStarted;
  }

  if (m_pImpl->m_exitCodeAvailable)
  {
    return ezProcessState::Finished;
  }

  int childStatus = -1;
  int waitResult = waitpid(m_pImpl->m_childPid, &childStatus, WNOHANG);
  if (waitResult > 0)
  {
    m_iExitCode = WEXITSTATUS(childStatus);
    m_pImpl->m_exitCodeAvailable = true;

    m_pImpl->StopStreamWatcher();

    return ezProcessState::Finished;
  }

  return ezProcessState::Running;
}

void ezProcess::Detach()
{
  m_pImpl->m_childPid = -1;
}

ezOsProcessHandle ezProcess::GetProcessHandle() const
{
  EZ_ASSERT_DEV(false, "There is no process handle on posix");
  return nullptr;
}

ezOsProcessID ezProcess::GetProcessID() const
{
  EZ_ASSERT_DEV(m_pImpl->m_childPid != -1, "No ProcessID available");
  return m_pImpl->m_childPid;
}

ezOsProcessID ezProcess::GetCurrentProcessID()
{
  return getpid();
}
