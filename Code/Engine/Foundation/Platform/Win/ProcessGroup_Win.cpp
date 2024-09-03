#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS) && EZ_ENABLED(EZ_SUPPORTS_PROCESSES)

#  include <Foundation/Logging/Log.h>
#  include <Foundation/System/ProcessGroup.h>

struct ezProcessGroupImpl
{
  HANDLE m_hJobObject = INVALID_HANDLE_VALUE;
  HANDLE m_hCompletionPort = INVALID_HANDLE_VALUE;
  ezString m_sName;

  ~ezProcessGroupImpl();
  void Close();
  void Initialize();
};

ezProcessGroupImpl::~ezProcessGroupImpl()
{
  Close();
}

void ezProcessGroupImpl::Close()
{
  if (m_hJobObject != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hJobObject);
    m_hJobObject = INVALID_HANDLE_VALUE;
  }
}

void ezProcessGroupImpl::Initialize()
{
  if (m_hJobObject == INVALID_HANDLE_VALUE)
  {
    m_hJobObject = CreateJobObjectW(nullptr, nullptr);

    if (m_hJobObject == nullptr || m_hJobObject == INVALID_HANDLE_VALUE)
    {
      ezLog::Error("Failed to create process group '{}' - {}", m_sName, ezArgErrorCode(GetLastError()));
      return;
    }

    // configure the job object such that it kill all processes once this job object is cleaned up
    // ie. either when all job object handles are closed, or the application crashes

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION exinfo = {};
    exinfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (SetInformationJobObject(m_hJobObject, JobObjectExtendedLimitInformation, &exinfo, sizeof(exinfo)) == FALSE)
    {
      ezLog::Error("ezProcessGroup: failed to configure 'kill jobs on close' - '{}'", ezArgErrorCode(GetLastError()));
    }

    // the completion port is necessary to implement WaitToFinish()
    // see https://devblogs.microsoft.com/oldnewthing/20130405-00/?p=4743
    m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);

    JOBOBJECT_ASSOCIATE_COMPLETION_PORT Port;
    Port.CompletionKey = m_hJobObject;
    Port.CompletionPort = m_hCompletionPort;
    SetInformationJobObject(m_hJobObject, JobObjectAssociateCompletionPortInformation, &Port, sizeof(Port));
  }
}

ezProcessGroup::ezProcessGroup(ezStringView sGroupName)
{
  m_pImpl = EZ_DEFAULT_NEW(ezProcessGroupImpl);
  m_pImpl->m_sName = sGroupName;
}

ezProcessGroup::~ezProcessGroup()
{
  TerminateAll().IgnoreResult();
}

ezResult ezProcessGroup::Launch(const ezProcessOptions& opt)
{
  m_pImpl->Initialize();

  ezProcess& process = m_Processes.ExpandAndGetRef();
  EZ_SUCCEED_OR_RETURN(process.Launch(opt, ezProcessLaunchFlags::Suspended));

  if (AssignProcessToJobObject(m_pImpl->m_hJobObject, process.GetProcessHandle()) == FALSE)
  {
    ezLog::Error("Failed to add process to process group '{}' - {}", m_pImpl->m_sName, ezArgErrorCode(GetLastError()));
    m_Processes.PopBack();
    return EZ_FAILURE;
  }

  if (process.ResumeSuspended().Failed())
  {
    ezLog::Error("Failed to resume the given process. Processes must be launched in a suspended state before adding them to process groups.");
    m_Processes.PopBack();
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezProcessGroup::WaitToFinish(ezTime timeout /*= ezTime::MakeZero()*/)
{
  if (m_pImpl->m_hJobObject == INVALID_HANDLE_VALUE)
    return EZ_SUCCESS;

  // check if no new processes were launched, because waiting could end up in an infinite loop,
  // so don't even try in this case
  bool allProcessesGone = true;
  for (const ezProcess& p : m_Processes)
  {
    DWORD exitCode = 0;
    GetExitCodeProcess(p.GetProcessHandle(), &exitCode);
    if (exitCode == STILL_ACTIVE)
    {
      allProcessesGone = false;
      break;
    }
  }

  if (allProcessesGone)
  {
    // We need to wait for processes even if the job is done as the threads for the pipes are potentially still alive and lead to incomplete stdout / stderr output even though the process has exited.
    for (ezProcess& p : m_Processes)
    {
      p.WaitToFinish().IgnoreResult();
    }
    m_pImpl->Close();
    return EZ_SUCCESS;
  }

  DWORD dwTimeout = INFINITE;

  if (timeout.IsPositive())
    dwTimeout = (DWORD)timeout.GetMilliseconds();
  else
    dwTimeout = INFINITE;

  DWORD CompletionCode;
  ULONG_PTR CompletionKey;
  LPOVERLAPPED Overlapped;

  ezTime tStart = ezTime::Now();

  while (true)
  {
    // ATTENTION !
    // If you are looking at a crash dump of ez this line will typically be at the top of the callstack.
    // That is because to write the crash dump an external process is called and this is where we are waiting for that process to finish.
    // To see the actual reason for the crash, locate the call to ezCrashHandlerFunc further down in the callstack.
    // The crashing code is usually the one calling that function.

    if (GetQueuedCompletionStatus(m_pImpl->m_hCompletionPort, &CompletionCode, &CompletionKey, &Overlapped, dwTimeout) == FALSE)
    {
      DWORD res = GetLastError();

      if (res != WAIT_TIMEOUT)
      {
        ezLog::Error("Failed to wait for process group '{}' - {}", m_pImpl->m_sName, ezArgErrorCode(res));
      }

      return EZ_FAILURE;
    }

    // we got the expected result, all processes have finished
    if (((HANDLE)CompletionKey == m_pImpl->m_hJobObject && CompletionCode == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO))
    {
      // We need to wait for processes even if the job is done as the threads for the pipes are potentially still alive and lead to incomplete stdout / stderr output even though the process has exited.
      for (ezProcess& p : m_Processes)
      {
        p.WaitToFinish().IgnoreResult();
      }

      m_pImpl->Close();
      return EZ_SUCCESS;
    }

    // we got some different message, ignore this
    // however, we need to adjust our timeout

    if (timeout.IsPositive())
    {
      // subtract the time that we spent
      const ezTime now = ezTime::Now();
      timeout -= now - tStart;
      tStart = now;

      // the timeout has been reached
      if (timeout.IsZeroOrNegative())
      {
        return EZ_FAILURE;
      }

      // otherwise try again, but with a reduced timeout
      dwTimeout = (DWORD)timeout.GetMilliseconds();
    }
  }
}

ezResult ezProcessGroup::TerminateAll(ezInt32 iForcedExitCode /*= -2*/)
{
  if (m_pImpl->m_hJobObject == INVALID_HANDLE_VALUE)
    return EZ_SUCCESS;

  if (TerminateJobObject(m_pImpl->m_hJobObject, (UINT)iForcedExitCode) == FALSE)
  {
    ezLog::Error("Failed to terminate process group '{}' - {}", m_pImpl->m_sName, ezArgErrorCode(GetLastError()));
    return EZ_FAILURE;
  }

  EZ_SUCCEED_OR_RETURN(WaitToFinish());

  return EZ_SUCCESS;
}

#endif
