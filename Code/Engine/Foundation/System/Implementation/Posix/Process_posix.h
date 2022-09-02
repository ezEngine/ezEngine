#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Logging/Log.h>
#include <Foundation/System/Process.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

EZ_DEFINE_AS_POD_TYPE(struct pollfd);

namespace
{
  ezResult AddFdFlags(int fd, int addFlags)
  {
    int flags = fcntl(fd, F_GETFD);
    flags |= addFlags;
    if (fcntl(fd, F_SETFD, flags) != 0)
    {
      ezLog::Error("Failed to set flags on {}: {}", fd, errno);
      return EZ_FAILURE;
    }
    return EZ_SUCCESS;
  }
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
    int fd;
    ezDelegate<void(ezStringView)> callback;
  };
  ezHybridArray<StdStreamInfo, 2> m_streams;
  ezDynamicArray<ezStringBuilder> m_overflowBuffers;
  ezUniquePtr<ezOSThread> m_streamWatcherThread;
  int m_wakeupPipeReadEnd = -1;
  int m_wakeupPipeWriteEnd = -1;

  static void* StreamWatcherThread(void* context)
  {
    ezProcessImpl* self = reinterpret_cast<ezProcessImpl*>(context);
    char buffer[4096];

    ezHybridArray<struct pollfd, 3> pollfds;

    pollfds.PushBack({self->m_wakeupPipeReadEnd, POLLIN, 0});
    for (StdStreamInfo& stream : self->m_streams)
    {
      pollfds.PushBack({stream.fd, POLLIN, 0});
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
          if (pollfds[i].revents != 0)
          {
            ezStringBuilder& overflowBuffer = self->m_overflowBuffers[i - 1];
            StdStreamInfo& stream = self->m_streams[i - 1];
            pollfds[i].revents = 0;
            while (true)
            {
              ssize_t numBytes = read(stream.fd, buffer, EZ_ARRAY_SIZE(buffer));
              if (numBytes < 0)
              {
                if (errno == EWOULDBLOCK)
                {
                  break;
                }
                ezLog::Error("Process Posix read error on {}: {}", stream.fd, errno);
                return nullptr;
              }
              if (numBytes == 0)
              {
                break;
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
            }
          }
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
    }

    return nullptr;
  }

  ezResult StartStreamWatcher()
  {
    int wakeupPipe[2] = {-1, -1};
    if (pipe(wakeupPipe) < 0)
    {
      ezLog::Error("Failed to setup wakeup pipe {}", errno);
      return EZ_FAILURE;
    }
    else
    {
      m_wakeupPipeReadEnd = wakeupPipe[0];
      m_wakeupPipeWriteEnd = wakeupPipe[1];
      if (AddFdFlags(m_wakeupPipeReadEnd, O_NONBLOCK | O_CLOEXEC).Failed() ||
          AddFdFlags(m_wakeupPipeWriteEnd, O_NONBLOCK | O_CLOEXEC).Failed())
      {
        close(m_wakeupPipeReadEnd);
        m_wakeupPipeReadEnd = -1;
        close(m_wakeupPipeWriteEnd);
        m_wakeupPipeWriteEnd = -1;
        return EZ_FAILURE;
      }
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
      EZ_IGNORE_UNUSED(write(m_wakeupPipeWriteEnd, &c, 1));
      m_streamWatcherThread->Join();
      m_streamWatcherThread = nullptr;
    }
    close(m_wakeupPipeReadEnd);
    close(m_wakeupPipeWriteEnd);
    m_wakeupPipeReadEnd = -1;
    m_wakeupPipeWriteEnd = -1;
  }

  void AddStream(int fd, const ezDelegate<void(ezStringView)>& callback)
  {
    m_streams.PushBack({fd, callback});
    m_overflowBuffers.SetCount(m_streams.GetCount());
  }

  static ezResult StartChildProcess(const ezProcessOptions& opt, pid_t& outPid, bool suspended, int& outStdOutFd, int& outStdErrFd)
  {
    int stdoutPipe[2] = {-1, -1};
    int stderrPipe[2] = {-1, -1};

    if (opt.m_onStdOut.IsValid())
    {
      if (pipe(stdoutPipe) < 0)
      {
        return EZ_FAILURE;
      }
    }

    if (opt.m_onStdError.IsValid())
    {
      if (pipe(stderrPipe) < 0)
      {
        return EZ_FAILURE;
      }
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
        close(stdoutPipe[0]);               // We don't need the read end of the pipe in the child process
        dup2(stdoutPipe[1], STDOUT_FILENO); // redirect the write end to STDOUT
        close(stdoutPipe[1]);
      }

      if (opt.m_onStdError.IsValid())
      {
        close(stderrPipe[0]);               // We don't need the read end of the pipe in the child process
        dup2(stderrPipe[1], STDERR_FILENO); // redirect the write end to STDERR
        close(stderrPipe[1]);
      }

      ezHybridArray<char*, 9> args;

      for (const ezString& arg : opt.m_Arguments)
      {
        args.PushBack(const_cast<char*>(arg.GetData()));
      }
      args.PushBack(nullptr);

      if (!opt.m_sWorkingDirectory.IsEmpty())
      {
        if (chdir(opt.m_sWorkingDirectory.GetData()) < 0)
        {
          _exit(-1); // Failed to change working directory
        }
      }

      if (execv(opt.m_sProcess.GetData(), args.GetData()) < 0)
      {
        _exit(-1);
      }
    }
    else
    {
      outPid = childPid;

      if (opt.m_onStdOut.IsValid())
      {
        close(stdoutPipe[1]); // Don't need the write end in the parent process
        outStdOutFd = stdoutPipe[0];
      }

      if (opt.m_onStdError.IsValid())
      {
        close(stderrPipe[1]); // Don't need the write end in the parent process
        outStdErrFd = stderrPipe[0];
      }
    }

    return EZ_SUCCESS;
  }
};

ezProcess::ezProcess()
{
  m_impl = EZ_DEFAULT_NEW(ezProcessImpl);
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
  m_impl.Clear();
}

ezResult ezProcess::Execute(const ezProcessOptions& opt, ezInt32* out_iExitCode /*= nullptr*/)
{
  pid_t childPid = 0;
  int stdoutFd = -1;
  int stderrFd = -1;
  if (ezProcessImpl::StartChildProcess(opt, childPid, false, stdoutFd, stderrFd).Failed())
  {
    return EZ_FAILURE;
  }

  ezProcessImpl impl;
  if (stdoutFd >= 0)
  {
    impl.AddStream(stdoutFd, opt.m_onStdOut);
  }

  if (stderrFd >= 0)
  {
    impl.AddStream(stderrFd, opt.m_onStdError);
  }

  if (stdoutFd >= 0 || stderrFd >= 0)
  {
    if (impl.StartStreamWatcher().Failed())
    {
      return EZ_FAILURE;
    }
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
    else
    {
      *out_iExitCode = -1;
    }
  }
  return EZ_SUCCESS;
}

ezResult ezProcess::Launch(const ezProcessOptions& opt, ezBitflags<ezProcessLaunchFlags> launchFlags /*= ezProcessLaunchFlags::None*/)
{
  EZ_ASSERT_DEV(m_impl->m_childPid == -1, "Can not reuse an instance of ezProcess");

  int stdoutFd = -1;
  int stderrFd = -1;

  if (ezProcessImpl::StartChildProcess(opt, m_impl->m_childPid, launchFlags.IsSet(ezProcessLaunchFlags::Suspended), stdoutFd, stderrFd).Failed())
  {
    return EZ_FAILURE;
  }

  m_impl->m_exitCodeAvailable = false;
  m_impl->m_processSuspended = launchFlags.IsSet(ezProcessLaunchFlags::Suspended);

  if (stdoutFd >= 0)
  {
    m_impl->AddStream(stdoutFd, opt.m_onStdOut);
  }

  if (stderrFd >= 0)
  {
    m_impl->AddStream(stderrFd, opt.m_onStdError);
  }

  if (stdoutFd >= 0 || stderrFd >= 0)
  {
    if (m_impl->StartStreamWatcher().Failed())
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
  if (m_impl->m_childPid < 0 || !m_impl->m_processSuspended)
  {
    return EZ_FAILURE;
  }

  if (kill(m_impl->m_childPid, SIGCONT) < 0)
  {
    return EZ_FAILURE;
  }
  m_impl->m_processSuspended = false;
  return EZ_SUCCESS;
}

ezResult ezProcess::WaitToFinish(ezTime timeout /*= ezTime::Zero()*/)
{
  int childStatus = 0;
  EZ_SCOPE_EXIT(m_impl->StopStreamWatcher());

  if (timeout.IsZero())
  {
    if (waitpid(m_impl->m_childPid, &childStatus, 0) < 0)
    {
      return EZ_FAILURE;
    }
  }
  else
  {
    int waitResult = 0;
    ezTime startWait = ezTime::Now();
    while (true)
    {
      waitResult = waitpid(m_impl->m_childPid, &childStatus, WNOHANG);
      if (waitResult < 0)
      {
        return EZ_FAILURE;
      }
      if (waitResult > 0)
      {
        break;
      }
      ezTime timeSpent = ezTime::Now() - startWait;
      if (timeSpent > timeout)
      {
        return EZ_FAILURE;
      }
      ezThreadUtils::Sleep(ezMath::Min(ezTime::Milliseconds(100.0), timeout - timeSpent));
    }
  }

  if (WIFEXITED(childStatus))
  {
    m_iExitCode = WEXITSTATUS(childStatus);
  }
  else
  {
    m_iExitCode = -1;
  }
  m_impl->m_exitCodeAvailable = true;
  return EZ_SUCCESS;
}

ezResult ezProcess::Terminate()
{
  if (m_impl->m_childPid == -1)
  {
    return EZ_FAILURE;
  }

  EZ_SCOPE_EXIT(m_impl->StopStreamWatcher());

  if (kill(m_impl->m_childPid, SIGKILL) < 0)
  {
    if (errno != ESRCH) // ESRCH = Process does not exist
    {
      return EZ_FAILURE;
    }
  }
  m_impl->m_exitCodeAvailable = true;
  m_iExitCode = -1;

  return EZ_SUCCESS;
}

ezProcessState ezProcess::GetState() const
{
  if (m_impl->m_childPid == -1)
  {
    return ezProcessState::NotStarted;
  }

  if (m_impl->m_exitCodeAvailable)
  {
    return ezProcessState::Finished;
  }

  int childStatus = -1;
  int waitResult = waitpid(m_impl->m_childPid, &childStatus, WNOHANG);
  if (waitResult > 0)
  {
    m_iExitCode = WEXITSTATUS(childStatus);
    m_impl->m_exitCodeAvailable = true;
    return ezProcessState::Finished;
  }

  return ezProcessState::Running;
}

void ezProcess::Detach()
{
  m_impl->m_childPid = -1;
}

ezOsProcessHandle ezProcess::GetProcessHandle() const
{
  EZ_ASSERT_DEV(false, "There is no process handle on posix");
  return nullptr;
}

ezOsProcessID ezProcess::GetProcessID() const
{
  EZ_ASSERT_DEV(m_impl->m_childPid != -1, "No ProcessID available");
  return m_impl->m_childPid;
}

ezOsProcessID ezProcess::GetCurrentProcessID()
{
  return getpid();
}
