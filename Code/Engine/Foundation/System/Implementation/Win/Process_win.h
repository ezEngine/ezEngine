#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Logging/Log.h>
#include <Foundation/System/Process.h>
#include <future>

struct ezPipeWin
{
  HANDLE m_pipeRead = nullptr;
  HANDLE m_pipeWrite = nullptr;
  std::thread m_readThread;

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

      if (m_readThread.joinable())
      {
        m_readThread.join();
      }
      CloseHandle(m_pipeRead);
      m_pipeRead = nullptr;
    }
  }

  void StartRead(ezDelegate<void(ezStringView)>& onStdOut)
  {
    if (m_pipeWrite)
    {
      m_readThread = std::thread([&]() {
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
  ezOsProcessHandle m_MainThreadHandle = nullptr;
  ezOsProcessID m_ProcessID = 0;
  ezPipeWin m_pipeStdOut;
  ezPipeWin m_pipeStdErr;

  ~ezProcessImpl() { Close(); }

  void Close()
  {
    if (m_MainThreadHandle != nullptr)
    {
      CloseHandle(m_MainThreadHandle);
      m_MainThreadHandle = nullptr;
    }

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
    ezLog::Dev("Process still running - terminating '{}'", m_sProcess);

    Terminate();
  }
}

ezOsProcessHandle ezProcess::GetProcessHandle() const
{
  return m_impl->m_ProcessHandle;
}

ezOsProcessID ezProcess::GetProcessID() const
{
  return m_impl->m_ProcessID;
}

ezOsProcessID ezProcess::GetCurrentProcessID()
{
  const ezOsProcessID processID = GetCurrentProcessId();
  return processID;
}


// Taken from "Programmatically controlling which handles are inherited by new processes in Win32" by Raymond Chen
// https://devblogs.microsoft.com/oldnewthing/20111216-00/?p=8873
static BOOL CreateProcessWithExplicitHandles(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes,
  LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
  LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
  // here is the new stuff
  DWORD cHandlesToInherit, HANDLE* rgHandlesToInherit)
{
  BOOL fSuccess;
  BOOL fInitialized = FALSE;
  SIZE_T size = 0;
  LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList = nullptr;
  fSuccess = cHandlesToInherit < 0xFFFFFFFF / sizeof(HANDLE) && lpStartupInfo->cb == sizeof(*lpStartupInfo);
  if (!fSuccess)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
  }

  if (cHandlesToInherit > 0)
  {
    if (fSuccess)
    {
      fSuccess = InitializeProcThreadAttributeList(nullptr, 1, 0, &size) || GetLastError() == ERROR_INSUFFICIENT_BUFFER;
    }
    if (fSuccess)
    {
      lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(HeapAlloc(GetProcessHeap(), 0, size));
      fSuccess = lpAttributeList != nullptr;
    }
    if (fSuccess)
    {
      fSuccess = InitializeProcThreadAttributeList(lpAttributeList, 1, 0, &size);
    }
    if (fSuccess)
    {
      fInitialized = TRUE;
      fSuccess = UpdateProcThreadAttribute(
        lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, rgHandlesToInherit, cHandlesToInherit * sizeof(HANDLE), nullptr, nullptr);
    }
  }

  if (fSuccess)
  {
    STARTUPINFOEXW info;
    ZeroMemory(&info, sizeof(info));
    info.StartupInfo = *lpStartupInfo;
    info.StartupInfo.cb = sizeof(info);
    info.lpAttributeList = lpAttributeList;

    // it is both possible to pass in (STARTUPINFOW*)&info OR info.StartupInfo ...
    fSuccess = CreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles,
      dwCreationFlags | EXTENDED_STARTUPINFO_PRESENT, lpEnvironment, lpCurrentDirectory, &info.StartupInfo, lpProcessInformation);
  }
  if (fInitialized)
    DeleteProcThreadAttributeList(lpAttributeList);
  if (lpAttributeList)
    HeapFree(GetProcessHeap(), 0, lpAttributeList);
  return fSuccess;
}

ezResult ezProcess::Launch(const ezProcessOptions& opt, ezBitflags<ezProcessLaunchFlags> launchFlags /*= ezAsyncProcessFlags::None*/)
{
  EZ_ASSERT_DEV(m_impl->m_ProcessHandle == nullptr, "Cannot reuse an instance of ezProcess");
  EZ_ASSERT_DEV(m_impl->m_ProcessID == 0, "Cannot reuse an instance of ezProcess");

  ezStringBuilder sProcess = opt.m_sProcess;
  sProcess.MakeCleanPath();
  sProcess.ReplaceAll("/", "\\");

  m_sProcess = sProcess;
  m_onStdOut = opt.m_onStdOut;
  m_onStdError = opt.m_onStdError;

  STARTUPINFOW si;
  ezMemoryUtils::ZeroFill(&si, 1);
  si.cb = sizeof(si);
  si.dwFlags = STARTF_FORCEOFFFEEDBACK; // do not show a wait cursor while launching the process

  // attention: passing in even a single null handle will fail the handle inheritance entirely,
  // but CreateProcess will still return success
  // therefore we must ensure to only pass non-null handles to inherit
  HANDLE HandlesToInherit[2];
  ezUInt32 uiNumHandlesToInherit = 0;

  if (m_onStdOut.IsValid())
  {
    m_impl->m_pipeStdOut.Create();
    si.hStdOutput = m_impl->m_pipeStdOut.m_pipeWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;
    HandlesToInherit[uiNumHandlesToInherit++] = m_impl->m_pipeStdOut.m_pipeWrite;
  }
  if (m_onStdError.IsValid())
  {
    m_impl->m_pipeStdErr.Create();
    si.hStdError = m_impl->m_pipeStdErr.m_pipeWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;
    HandlesToInherit[uiNumHandlesToInherit++] = m_impl->m_pipeStdErr.m_pipeWrite;
  }

  PROCESS_INFORMATION pi;
  ezMemoryUtils::ZeroFill(&pi, 1);


  ezStringBuilder sCmdLine;
  BuildFullCommandLineString(opt, sProcess, sCmdLine);

  DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT;

  if (opt.m_bHideConsoleWindow)
  {
    dwCreationFlags |= CREATE_NO_WINDOW;
  }

  if (launchFlags.IsSet(ezProcessLaunchFlags::Suspended))
  {
    dwCreationFlags |= CREATE_SUSPENDED;
  }


  if (!CreateProcessWithExplicitHandles(ezStringWChar(sProcess).GetData(), const_cast<wchar_t*>(ezStringWChar(sCmdLine).GetData()),
        nullptr,                              // lpProcessAttributes
        nullptr,                              // lpThreadAttributes
        uiNumHandlesToInherit > 0 ? TRUE : FALSE, // bInheritHandles
        dwCreationFlags,
        nullptr,           // lpEnvironment
        nullptr,           // lpCurrentDirectory
        &si,               // lpStartupInfo
        &pi,               // lpProcessInformation
        uiNumHandlesToInherit, // cHandlesToInherit
        HandlesToInherit   // rgHandlesToInherit
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

  if (launchFlags.IsSet(ezProcessLaunchFlags::Suspended))
  {
    // store the main thread handle for ResumeSuspended() later
    m_impl->m_MainThreadHandle = pi.hThread;
  }
  else
  {
    CloseHandle(pi.hThread);
  }

  if (launchFlags.IsSet(ezProcessLaunchFlags::Detached))
  {
    Detach();
  }

  return EZ_SUCCESS;
}

ezResult ezProcess::ResumeSuspended()
{
  if (m_impl->m_ProcessHandle == nullptr || m_impl->m_MainThreadHandle == nullptr)
    return EZ_FAILURE;

  ResumeThread(m_impl->m_MainThreadHandle);

  // invalidate the thread handle, so that we cannot resume the process twice
  CloseHandle(m_impl->m_MainThreadHandle);
  m_impl->m_MainThreadHandle = nullptr;

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
    ezLog::Error("Failed to wait for '{}' - {}", m_sProcess, ezArgErrorCode(GetLastError()));
    return EZ_FAILURE;
  }

  // the process has finished

  m_impl->m_pipeStdOut.Close();
  m_impl->m_pipeStdErr.Close();

  GetExitCodeProcess(m_impl->m_ProcessHandle, reinterpret_cast<DWORD*>(&m_iExitCode));

  return EZ_SUCCESS;
}

ezResult ezProcess::Execute(const ezProcessOptions& opt, ezInt32* out_iExitCode /*= nullptr*/)
{
  ezProcess proc;

  EZ_SUCCEED_OR_RETURN(proc.Launch(opt));
  EZ_SUCCEED_OR_RETURN(proc.WaitToFinish());

  if (out_iExitCode != nullptr)
  {
    *out_iExitCode = proc.GetExitCode();
  }

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
