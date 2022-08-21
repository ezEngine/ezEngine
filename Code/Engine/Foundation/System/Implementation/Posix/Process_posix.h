#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/Process.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Logging/Log.h>

#include <sys/wait.h>
#include <unistd.h>

struct ezProcessImpl
{
  EZ_DECLARE_POD_TYPE();

  pid_t m_childPid = -1;
  bool m_exitCodeAvailable = false;
  bool m_processSuspended = false;

  static ezResult StartChildProcess(const ezProcessOptions& opt, pid_t& outPid, bool suspended)
  {
    pid_t childPid = fork();
    if (childPid < 0)
    {
      return EZ_FAILURE;
    }

    if (childPid == 0) // We are the child
    {
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

      if(suspended)
      {
        if(raise(SIGSTOP) < 0)
        {
          _exit(-1);
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
  if (ezProcessImpl::StartChildProcess(opt, childPid, false).Failed())
  {
    return EZ_FAILURE;
  }

  int childReturnValue = -1;
  pid_t waitedPid = waitpid(childPid, &childReturnValue, 0);
  if (waitedPid < 0)
  {
    return EZ_FAILURE;
  }
  if (out_iExitCode != nullptr)
  {
    *out_iExitCode = childReturnValue;
  }
  return EZ_SUCCESS;
}

ezResult ezProcess::Launch(const ezProcessOptions& opt, ezBitflags<ezProcessLaunchFlags> launchFlags /*= ezProcessLaunchFlags::None*/)
{
  EZ_ASSERT_DEV(m_impl->m_childPid == -1, "Can not reuse an instance of ezProcess");

  if(ezProcessImpl::StartChildProcess(opt, m_impl->m_childPid, launchFlags.IsSet(ezProcessLaunchFlags::Suspended)).Failed())
  {
    return EZ_FAILURE;
  }

  m_impl->m_exitCodeAvailable = false;
  m_impl->m_processSuspended = launchFlags.IsSet(ezProcessLaunchFlags::Suspended);

  if(launchFlags.IsSet(ezProcessLaunchFlags::Detached))
  {
    Detach();
  }

  return EZ_SUCCESS;
}

ezResult ezProcess::ResumeSuspended()
{
  if(m_impl->m_childPid < 0 || !m_impl->m_processSuspended)
  {
    return EZ_FAILURE;
  }

  if(kill(m_impl->m_childPid, SIGCONT) < 0)
  {
    return EZ_FAILURE;
  }
  m_impl->m_processSuspended = false;
  return EZ_SUCCESS;
}

ezResult ezProcess::WaitToFinish(ezTime timeout /*= ezTime::Zero()*/)
{
  int returnValue = 0;
  if(timeout.IsZero())
  {
    if(waitpid(m_impl->m_childPid, &returnValue, 0) < 0)
    {
      return EZ_FAILURE;
    }
  }
  else 
  {
    int waitResult = 0;
    ezTime startWait = ezTime::Now();
    while(true)
    {
      waitResult = waitpid(m_impl->m_childPid, &returnValue, WNOHANG);
      if(waitResult < 0)
      {
        return EZ_FAILURE;
      }
      if(waitResult > 0)
      {
        break;
      }
      ezTime timeSpent = ezTime::Now() - startWait;
      if(timeSpent > timeout)
      {
        return EZ_FAILURE;
      }
      ezThreadUtils::Sleep(ezMath::Min(ezTime::Milliseconds(100.0), timeout - timeSpent));
    }
  }

  m_iExitCode = returnValue;
  m_impl->m_exitCodeAvailable = true;
  return EZ_SUCCESS;
}

ezResult ezProcess::Terminate()
{
  if(m_impl->m_childPid == -1)
  {
    return EZ_FAILURE;
  }

  if(kill(m_impl->m_childPid, SIGKILL) < 0)
  {
    if(errno != ESRCH) // ESRCH = Process does not exist
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
  if(m_impl->m_childPid == -1)
  {
    return ezProcessState::NotStarted;
  }

  if(m_impl->m_exitCodeAvailable)
  {
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
