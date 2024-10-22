#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <signal.h>
#endif

bool ezEngineProcessCommunicationChannel::IsHostAlive() const
{
  if (ezEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return true;

  if (m_iHostPID == 0)
    return false;

  bool bValid = true;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  DWORD pid = static_cast<DWORD>(m_iHostPID);
  HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  bValid = (hProcess != INVALID_HANDLE_VALUE) && (hProcess != nullptr);

  DWORD exitcode = 0;
  if (GetExitCodeProcess(hProcess, &exitcode) && exitcode != STILL_ACTIVE)
    bValid = false;

  CloseHandle(hProcess);
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
  // We send the signal 0 to the given PID (signal 0 is a no-op)
  // If this succeeds, the process with the given PID exists
  // if it fails, the process does not / no longer exist.
  if (kill(m_iHostPID, 0) < 0)
    bValid = false;
#else
#  error Not implemented
#endif

  return bValid;
}

ezResult ezEngineProcessCommunicationChannel::ConnectToHostProcess()
{
  EZ_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");

  if (!ezEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
  {
    if (ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-IPC").IsEmpty())
    {
      EZ_REPORT_FAILURE("Command Line does not contain -IPC parameter");
      return EZ_FAILURE;
    }

    if (ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-PID").IsEmpty())
    {
      EZ_REPORT_FAILURE("Command Line does not contain -PID parameter");
      return EZ_FAILURE;
    }

    m_iHostPID = 0;
    EZ_SUCCEED_OR_RETURN(ezConversionUtils::StringToInt64(ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-PID"), m_iHostPID));

    ezLog::Debug("Host Process ID: {0}", m_iHostPID);

    m_pChannel = ezIpcChannel::CreatePipeChannel(ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-IPC"), ezIpcChannel::Mode::Client);
  }
  else
  {
    m_pChannel = ezIpcChannel::CreateNetworkChannel("localhost:1050", ezIpcChannel::Mode::Server);
  }
  m_pProtocol = EZ_DEFAULT_NEW(ezIpcProcessMessageProtocol, m_pChannel.Borrow());
  m_pProtocol->m_MessageEvent.AddEventHandler(ezMakeDelegate(&ezProcessCommunicationChannel::MessageFunc, this));
  m_pChannel->Connect();

  return EZ_SUCCESS;
}
