#include <Foundation/Logging/Log.h>
#include <Foundation/System/Process.h>
#include <future>

struct ezPipeWin
{
  HANDLE m_pipeRead = nullptr;
  HANDLE m_pipeWrite = nullptr;
  std::future<void> m_read;

  void Create()
  {
    SECURITY_ATTRIBUTES saAttr;

    // Set the bInheritHandle flag so pipe handles are inherited.
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = nullptr;

    // Create a pipe for the child process.
    if (!CreatePipe(&m_pipeRead, &m_pipeWrite, &saAttr, 0))
      ezLog::Error("ezPipeWin: CreatePipe failed");

    // Ensure the read handle to the pipe is not inherited.
    if (!SetHandleInformation(m_pipeRead, HANDLE_FLAG_INHERIT, 0))
      ezLog::Error("Stdout SetHandleInformation");
  }

  void Close()
  {
    if (m_pipeWrite)
    {
      CloseHandle(m_pipeWrite);
      m_pipeWrite = nullptr;

      m_read.wait();
      CloseHandle(m_pipeRead);
      m_pipeRead = nullptr;
    }
  }

  void StartRead(ezDelegate<void(ezStringView)>& onStdOut)
  {
    if (m_pipeWrite)
    {
      m_read = std::async(std::launch::async, [&]() {
        constexpr int BUFSIZE = 512;
        char chBuf[BUFSIZE];
        while (true)
        {
          DWORD bytesRead = 0;
          bool res = ReadFile(m_pipeRead, chBuf, BUFSIZE, &bytesRead, nullptr);
          if (!res || bytesRead == 0)
            break;

          onStdOut(ezStringView(chBuf, chBuf + bytesRead));
        }
      });
    }
  }
};

struct ezProcessImpl
{
  ezOsProcessHandle m_ProcessHandle = nullptr;
  ezOsProcessID m_ProcessID = 0;
  ezPipeWin m_pipeStdOut;
  ezPipeWin m_pipeStdErr;

  ~ezProcessImpl() { Close(); }

  void Close()
  {
    if (m_ProcessHandle != nullptr)
    {
      CloseHandle(m_ProcessHandle);
      m_ProcessHandle = nullptr;
    }

    m_pipeStdOut.Close();
    m_pipeStdErr.Close();
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
    Terminate();
  }
}

ezResult ezProcess::LaunchAsync()
{
  EZ_ASSERT_DEV(m_impl->m_ProcessHandle == nullptr, "Cannot reuse an instance of ezProcess");
  EZ_ASSERT_DEV(m_impl->m_ProcessID == 0, "Cannot reuse an instance of ezProcess");

  ezStringBuilder sProcess = m_sProcess;
  sProcess.MakeCleanPath();
  sProcess.ReplaceAll("/", "\\");

  STARTUPINFOW si;
  ezMemoryUtils::ZeroFill(&si, 1);
  si.cb = sizeof(si);
  si.dwFlags = STARTF_FORCEOFFFEEDBACK; // do not show a wait cursor while launching the process

  if (m_onStdOut.IsValid())
  {
    m_impl->m_pipeStdOut.Create();
    si.hStdOutput = m_impl->m_pipeStdOut.m_pipeWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;
  }
  if (m_onStdError.IsValid())
  {
    m_impl->m_pipeStdErr.Create();
    si.hStdError = m_impl->m_pipeStdErr.m_pipeWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;
  }

  PROCESS_INFORMATION pi;
  ezMemoryUtils::ZeroFill(&pi, 1);


  ezStringBuilder sCmdLine;
  BuildCommandLineString(sProcess, sCmdLine);

  DWORD dwCreationFlags = 0;

  if (m_bHideConsoleWindow)
  {
    dwCreationFlags |= CREATE_NO_WINDOW;
  }
  if (!CreateProcessW(ezStringWChar(sProcess).GetData(), const_cast<wchar_t*>(ezStringWChar(sCmdLine).GetData()),
        nullptr, // lpProcessAttributes
        nullptr, // lpThreadAttributes
        (si.dwFlags & STARTF_USESTDHANDLES) != 0 ? TRUE : FALSE,    // bInheritHandles
        dwCreationFlags,
        nullptr, // lpEnvironment
        nullptr, // lpCurrentDirectory
        &si,     // lpStartupInfo
        &pi      // lpProcessInformation
        ))
  {
    m_impl->m_pipeStdOut.Close();
    m_impl->m_pipeStdErr.Close();
    ezLog::Error("Failed to launch '{} {}' - {}", sProcess, sCmdLine, ezArgErrorCode(GetLastError()));
    return EZ_FAILURE;
  }
  m_impl->m_pipeStdOut.StartRead(m_onStdOut);
  m_impl->m_pipeStdErr.StartRead(m_onStdError);

  m_impl->m_ProcessHandle = pi.hProcess;
  m_impl->m_ProcessID = pi.dwProcessId;

  return EZ_SUCCESS;
}

ezResult ezProcess::WaitToFinish(ezTime timeout /*= ezTime::Zero()*/)
{
  EZ_ASSERT_DEV(m_impl->m_ProcessHandle != nullptr, "Launch a process before waiting on it");
  EZ_ASSERT_DEV(m_impl->m_ProcessID != 0, "Launch a process before waiting on it");

  DWORD dwTimeout = INFINITE;

  if (timeout.IsPositive())
    dwTimeout = (DWORD)timeout.GetMilliseconds();
  else
    dwTimeout = INFINITE;

  const DWORD res = WaitForSingleObject(m_impl->m_ProcessHandle, dwTimeout);

  if (res == WAIT_TIMEOUT)
  {
    // the process is not yet finished, the timeout was reached
    return EZ_FAILURE;
  }

  if (res == WAIT_FAILED)
  {
    ezStringBuilder sCmdLine;
    BuildCommandLineString(m_sProcess, sCmdLine);

    ezLog::Error("Failed to wait for '{} {}' - {}", m_sProcess, sCmdLine, ezArgErrorCode(GetLastError()));
    return EZ_FAILURE;
  }

  // the process has finished

  m_impl->m_pipeStdOut.Close();
  m_impl->m_pipeStdErr.Close();

  GetExitCodeProcess(m_impl->m_ProcessHandle, reinterpret_cast<DWORD*>(&m_iExitCode));

  return EZ_SUCCESS;
}

ezResult ezProcess::Launch()
{
  EZ_SUCCEED_OR_RETURN(LaunchAsync());
  EZ_SUCCEED_OR_RETURN(WaitToFinish());

  return EZ_SUCCESS;
}

ezResult ezProcess::Terminate()
{
  EZ_ASSERT_DEV(m_impl->m_ProcessHandle != nullptr, "Launch a process before terminating it");
  EZ_ASSERT_DEV(m_impl->m_ProcessID != 0, "Launch a process before terminating it");

  if (TerminateProcess(m_impl->m_ProcessHandle, 0xFFFFFFFF) == FALSE)
  {
    ezLog::Error("Failed to terminate process '{}' - {}", m_sProcess, ezArgErrorCode(GetLastError()));
    return EZ_FAILURE;
  }

  EZ_SUCCEED_OR_RETURN(WaitToFinish());

  return EZ_SUCCESS;
}

ezProcessState ezProcess::GetState() const
{
  if (m_impl->m_ProcessHandle == 0)
    return ezProcessState::NotStarted;

  DWORD exitCode = 0;
  if (GetExitCodeProcess(m_impl->m_ProcessHandle, &exitCode) == FALSE)
  {
    ezLog::Error("Failed to retrieve exit code for process '{}' - {}", m_sProcess, ezArgErrorCode(GetLastError()));

    // not sure what kind of errors can happen (probably access denied and such)
    // have to return something, so lets claim the process is finished
    return ezProcessState::Finished;
  }

  if (exitCode == STILL_ACTIVE)
    return ezProcessState::Running;

  m_iExitCode = (ezInt32)exitCode;
  return ezProcessState::Finished;
}

void ezProcess::Detach()
{
  // throw away the previous ezProcessImpl and create a blank one
  m_impl = EZ_DEFAULT_NEW(ezProcessImpl);

  // reset the exit code to the default
  m_iExitCode = -0xFFFF;
}
