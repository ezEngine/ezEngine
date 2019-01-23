#include <PCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>

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
#endif

  return bValid;
}

ezResult ezEngineProcessCommunicationChannel::ConnectToHostProcess()
{
  EZ_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");

  if (!ezEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
  {
    if (ezStringUtils::IsNullOrEmpty(ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-IPC")))
    {
      EZ_REPORT_FAILURE("Command Line does not contain -IPC parameter");
      return EZ_FAILURE;
    }

    if (ezStringUtils::IsNullOrEmpty(ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-PID")))
    {
      EZ_REPORT_FAILURE("Command Line does not contain -PID parameter");
      return EZ_FAILURE;
    }

    m_iHostPID = 0;
    ezConversionUtils::StringToInt64(ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-PID"), m_iHostPID);

    ezLog::Debug("Host Process ID: {0}", m_iHostPID);

    m_pChannel =
        ezIpcChannel::CreatePipeChannel(ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-IPC"), ezIpcChannel::Mode::Client);
  }
  else
  {
    m_pChannel = ezIpcChannel::CreateNetworkChannel("localhost:1050", ezIpcChannel::Mode::Server);
  }

  m_pChannel->m_MessageEvent.AddEventHandler(ezMakeDelegate(&ezProcessCommunicationChannel::MessageFunc, this));
  m_pChannel->Connect();

  return EZ_SUCCESS;
}
