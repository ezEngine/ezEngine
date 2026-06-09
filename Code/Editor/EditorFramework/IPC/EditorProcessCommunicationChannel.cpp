#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/Process.h>

ezResult ezEditorProcessCommunicationChannel::StartClientProcess(const char* szProcess, const QStringList& args, bool bRemote, const ezRTTI* pFirstAllowedMessageType, ezUInt32 uiMemSize)
{
  EZ_LOG_BLOCK("ezProcessCommunicationChannel::StartClientProcess");

  EZ_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");
  EZ_ASSERT_DEV(m_pClientProcessGroup == nullptr, "ProcessCommunication object already in use");

  m_pFirstAllowedMessageType = pFirstAllowedMessageType;

  static ezUInt64 uiUniqueHash = 0;
  ezOsProcessID PID = ezProcess::GetCurrentProcessID();
  uiUniqueHash = ezHashingUtils::xxHash64(&PID, sizeof(PID), uiUniqueHash);
  ezTime time = ezTime::Now();
  uiUniqueHash = ezHashingUtils::xxHash64(&time, sizeof(time), uiUniqueHash);
  ezStringBuilder sMemName;
  sMemName.SetFormat("{0}", ezArgU(uiUniqueHash, 16, true, 16, true));
  ++uiUniqueHash;

  ezResult res = EZ_SUCCESS;
  if (bRemote)
  {
    res = CreateAndConnectChannel(ezIpcChannel::CreateNetworkChannel("172.16.80.3:1050", ezIpcChannel::Mode::Client));
  }
  else
  {
    res = CreateAndConnectChannel(ezIpcChannel::CreatePipeChannel(sMemName, ezIpcChannel::Mode::Server));
  }
  if (res.Failed())
  {
    ezLog::Error("IpcChannel: CreateAndConnectChannel failed");
    CloseConnection();
    return EZ_FAILURE;
  }

  for (ezUInt32 i = 0; i < 100; i++)
  {
    if (m_pChannel->GetConnectionState() == ezIpcChannel::ConnectionState::Connecting)
      break;

    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));
  }
  if (m_pChannel->GetConnectionState() != ezIpcChannel::ConnectionState::Connecting)
  {
    ezLog::Error("Failed to start IPC server");
    CloseConnection();
    return EZ_FAILURE;
  }

  ezStringBuilder sPath = szProcess;

  if (!sPath.IsAbsolutePath())
  {
    sPath = ezOSFile::GetApplicationDirectory();
    sPath.AppendPath(szProcess);
  }

  sPath.MakeCleanPath();

  if (!bRemote)
  {
    ezProcessOptions po;
    po.m_sProcess = sPath;
    po.AddArgument("-IPC");
    po.AddArgument(sMemName);
    po.AddArgument("-PID");
    po.AddArgument("{}", ezArgU(ezProcess::GetCurrentProcessID(), 1, false));
    for (const QString& arg : args)
      po.AddArgument(ezStringView(arg.toUtf8().constData()));

    m_pClientProcessGroup = EZ_DEFAULT_NEW(ezProcessGroup);
    if (m_pClientProcessGroup->Launch(po).Failed())
    {
      CloseConnection();
      ezLog::Error("Failed to start process '{0}'", sPath);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

bool ezEditorProcessCommunicationChannel::IsClientAlive() const
{
  if (m_pClientProcessGroup == nullptr)
    return false;
  const auto& processes = m_pClientProcessGroup->GetProcesses();
  if (processes.IsEmpty())
    return false;
  return processes[0].GetState() == ezProcessState::Running;
}

void ezEditorProcessCommunicationChannel::CloseConnection()
{
  DestroyChannel();
  m_pClientProcessGroup = nullptr;
}

ezString ezEditorProcessCommunicationChannel::GetStdoutContents()
{
  return ezString();
}

ezOsProcessID ezEditorProcessCommunicationChannel::GetProcessId() const
{
  if (m_pClientProcessGroup == nullptr)
    return {};
  const auto& processes = m_pClientProcessGroup->GetProcesses();
  if (!processes.IsEmpty() && processes[0].GetState() == ezProcessState::Running)
    return processes[0].GetProcessID();
  return {};
}

//////////////////////////////////////////////////////////////////////////

ezResult ezEditorProcessRemoteCommunicationChannel::ConnectToServer(const char* szAddress)
{
  EZ_LOG_BLOCK("ezEditorProcessRemoteCommunicationChannel::ConnectToServer");
  EZ_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");
  m_pFirstAllowedMessageType = nullptr;
  if (CreateAndConnectChannel(ezIpcChannel::CreateNetworkChannel(szAddress, ezIpcChannel::Mode::Client)).Failed())
  {
    ezLog::Error("IpcChannel: CreateAndConnectChannel failed");
    CloseConnection();
    return EZ_FAILURE;
  }

  for (ezUInt32 i = 0; i < 200; i++)
  {
    if (m_pChannel->GetConnectionState() != ezIpcChannel::ConnectionState::Connecting)
      break;

    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));
  }

  if (m_pChannel->GetConnectionState() != ezIpcChannel::ConnectionState::Connected)
  {
    ezLog::Error("Failed to connect to IPC server");
    CloseConnection();
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

bool ezEditorProcessRemoteCommunicationChannel::IsConnected() const
{
  return m_pChannel->IsConnected();
}

void ezEditorProcessRemoteCommunicationChannel::CloseConnection()
{
  DestroyChannel();
}

void ezEditorProcessRemoteCommunicationChannel::TryConnect()
{
  if (m_pChannel && m_pChannel->GetConnectionState() == ezIpcChannel::ConnectionState::Disconnected)
  {
    m_pChannel->Connect().IgnoreResult();
  }
}
