#include <PCH.h>
#include <EditorFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Logging/Log.h>
#include <QCoreApplication>
#include <QProcess>

ezProcessCommunicationChannel::ezProcessCommunicationChannel()
{
  m_pClientProcess = nullptr;
  m_uiProcessID = 0;
  m_pWaitForMessageType = nullptr;
  m_pChannel = nullptr;
}


ezProcessCommunicationChannel::~ezProcessCommunicationChannel()
{
  if (m_pChannel)
  {
    EZ_DEFAULT_DELETE(m_pChannel);
  }
}

ezResult ezProcessCommunicationChannel::StartClientProcess(const char* szProcess, const QStringList& args, const ezRTTI* pFirstAllowedMessageType, ezUInt32 uiMemSize)
{
  EZ_LOG_BLOCK("ezProcessCommunicationChannel::StartClientProcess");

  EZ_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");
  EZ_ASSERT_DEV(m_pClientProcess == nullptr, "ProcessCommunication object already in use");

  m_uiProcessID = 0xF0F0F0F0;

  m_pFirstAllowedMessageType = pFirstAllowedMessageType;

  static ezUInt64 uiUniqueHash = 0;
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  DWORD PID = GetCurrentProcessId();
  uiUniqueHash = ezHashing::MurmurHash64(&PID, sizeof(PID), uiUniqueHash);
#endif
  ezTime time = ezTime::Now();
  uiUniqueHash = ezHashing::MurmurHash64(&time, sizeof(time), uiUniqueHash);
  ezStringBuilder sMemName;
  sMemName.Format("{0}", ezArgU(uiUniqueHash, 16, false, 16, true));
  ++uiUniqueHash;

  m_pChannel = ezIpcChannel::CreatePipeChannel(sMemName, ezIpcChannel::Mode::Server);
  m_pChannel->m_MessageEvent.AddEventHandler(ezMakeDelegate(&ezProcessCommunicationChannel::MessageFunc, this));
  m_pChannel->Connect();

  ezStringBuilder sPath = szProcess;

  if (!sPath.IsAbsolutePath())
  {
    sPath = ezOSFile::GetApplicationDirectory();
    sPath.AppendPath(szProcess);
  }

  sPath.MakeCleanPath();

  ezStringBuilder sPID;
  ezConversionUtils::ToString((ezUInt64)QCoreApplication::applicationPid(), sPID);

  QStringList arguments;
  arguments << "-IPC";
  arguments << QLatin1String(sMemName.GetData());
  arguments << "-PID";
  arguments << sPID.GetData();
  arguments.append(args);

  m_pClientProcess = new QProcess();
  m_pClientProcess->start(QString::fromUtf8(sPath.GetData()), arguments, QIODevice::OpenModeFlag::NotOpen);

  if (!m_pClientProcess->waitForStarted())
  {
    delete m_pClientProcess;
    m_pClientProcess = nullptr;

    EZ_DEFAULT_DELETE(m_pChannel);

    ezLog::Error("Failed to start process '{0}'", sPath.GetData());
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezProcessCommunicationChannel::ConnectToHostProcess()
{
  EZ_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");
  EZ_ASSERT_DEV(m_pClientProcess == nullptr, "ProcessCommunication object already in use");

  m_uiProcessID = 0x0A0A0A0A;

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

  m_pChannel = ezIpcChannel::CreatePipeChannel(ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-IPC"), ezIpcChannel::Mode::Client);
  m_pChannel->m_MessageEvent.AddEventHandler(ezMakeDelegate(&ezProcessCommunicationChannel::MessageFunc, this));
  m_pChannel->Connect();

  return EZ_SUCCESS;
}

void ezProcessCommunicationChannel::CloseConnection()
{
  m_uiProcessID = 0;

  EZ_DEFAULT_DELETE(m_pChannel);

  m_pClientProcess->close();
  delete m_pClientProcess;
  m_pClientProcess = nullptr;
}

bool ezProcessCommunicationChannel::IsHostAlive() const
{
  if (m_iHostPID == 0)
    return false;

  bool bValid = true;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_iHostPID);
  bValid = (hProcess != INVALID_HANDLE_VALUE) && (hProcess != nullptr);

  DWORD exitcode = 0;
  if (GetExitCodeProcess(hProcess, &exitcode) && exitcode != STILL_ACTIVE)
    bValid = false;

  CloseHandle(hProcess);
#endif

  return bValid;
}

bool ezProcessCommunicationChannel::IsClientAlive() const
{
  if (m_pClientProcess == nullptr)
    return false;

  bool bRunning = m_pClientProcess->state() != QProcess::NotRunning;
  bool bNoError = m_pClientProcess->error() == QProcess::UnknownError;

  return bRunning && bNoError;
}

void ezProcessCommunicationChannel::SendMessage(ezProcessMessage* pMessage)
{
  if (m_pFirstAllowedMessageType != nullptr)
  {
    // ignore all messages that are not the first allowed message
    // this is necessary to make sure that during an engine restart we don't accidentally send stray messages while
    // the engine is not yet correctly set up
    if (!pMessage->GetDynamicRTTI()->IsDerivedFrom(m_pFirstAllowedMessageType))
      return;

    m_pFirstAllowedMessageType = nullptr;
  }

  {
    if (m_pChannel == nullptr)
      return;

    m_pChannel->Send(pMessage);
  }
}

bool ezProcessCommunicationChannel::ProcessMessages()
{
  if (!m_pChannel)
    return false;

  return m_pChannel->ProcessMessages();
}


void ezProcessCommunicationChannel::WaitForMessages()
{
  if (!m_pChannel)
    return;

  m_pChannel->WaitForMessages();
}

void ezProcessCommunicationChannel::MessageFunc(const ezProcessMessage* msg)
{
  const ezRTTI* pRtti = msg->GetDynamicRTTI();

  if (m_pWaitForMessageType != nullptr && msg->GetDynamicRTTI()->IsDerivedFrom(m_pWaitForMessageType))
  {
    if (m_WaitForMessageCallback.IsValid())
    {
      if (m_WaitForMessageCallback(const_cast<ezProcessMessage*>(msg)))
      {
        m_WaitForMessageCallback = WaitForMessageCallback();
        m_pWaitForMessageType = nullptr;
      }
    }
    else
    {
      m_pWaitForMessageType = nullptr;
    }
  }

  EZ_ASSERT_DEV(pRtti != nullptr, "Message Type unknown");
  EZ_ASSERT_DEV(msg != nullptr, "Object could not be allocated");
  EZ_ASSERT_DEV(pRtti->IsDerivedFrom<ezProcessMessage>(), "Msg base type is invalid");

  Event e;
  e.m_pMessage = msg;
  m_Events.Broadcast(e);
}

ezResult ezProcessCommunicationChannel::WaitForMessage(const ezRTTI* pMessageType, ezTime tTimeout, WaitForMessageCallback* pMessageCallack)
{
  EZ_ASSERT_DEV(m_pChannel != nullptr, "Need to connect first before waiting for a message.");
  //EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "This function is not thread safe");
  EZ_ASSERT_DEV(m_pWaitForMessageType == nullptr, "Already waiting for another message!");

  m_pWaitForMessageType = pMessageType;
  if (pMessageCallack)
  {
    m_WaitForMessageCallback = *pMessageCallack;
  }
  else
  {
    m_WaitForMessageCallback = WaitForMessageCallback();
  }

  const ezTime tStart = ezTime::Now();

  while (m_pWaitForMessageType != nullptr)
  {
    m_pChannel->WaitForMessages();

    if (tTimeout != ezTime())
    {
      if (ezTime::Now() - tStart > tTimeout)
      {
        m_pWaitForMessageType = nullptr;
        ezLog::Error("Reached time-out of {0} seconds while waiting for {1}", ezArgF(tTimeout.GetSeconds(), 1), pMessageType->GetTypeName());
        return EZ_FAILURE;
      }
    }
  }

  return EZ_SUCCESS;
}
