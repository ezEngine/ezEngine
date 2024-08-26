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
  EZ_ASSERT_DEV(m_pClientProcess == nullptr, "ProcessCommunication object already in use");

  m_pFirstAllowedMessageType = pFirstAllowedMessageType;

  static ezUInt64 uiUniqueHash = 0;
  ezOsProcessID PID = ezProcess::GetCurrentProcessID();
  uiUniqueHash = ezHashingUtils::xxHash64(&PID, sizeof(PID), uiUniqueHash);
  ezTime time = ezTime::Now();
  uiUniqueHash = ezHashingUtils::xxHash64(&time, sizeof(time), uiUniqueHash);
  ezStringBuilder sMemName;
  sMemName.SetFormat("{0}", ezArgU(uiUniqueHash, 16, false, 16, true));
  ++uiUniqueHash;

  if (bRemote)
  {
    m_pChannel = ezIpcChannel::CreateNetworkChannel("172.16.80.3:1050", ezIpcChannel::Mode::Client);
  }
  else
  {
    m_pChannel = ezIpcChannel::CreatePipeChannel(sMemName, ezIpcChannel::Mode::Server);
  }
  m_pProtocol = EZ_DEFAULT_NEW(ezIpcProcessMessageProtocol, m_pChannel.Borrow());
  m_pProtocol->m_MessageEvent.AddEventHandler(ezMakeDelegate(&ezProcessCommunicationChannel::MessageFunc, this));
  m_pChannel->Connect();
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

  ezStringBuilder sPID;
  ezConversionUtils::ToString((ezUInt64)QCoreApplication::applicationPid(), sPID);

  QStringList arguments;
  arguments << "-IPC";
  arguments << QLatin1String(sMemName.GetData());
  arguments << "-PID";
  arguments << sPID.GetData();
  arguments.append(args);

  m_pClientProcess = new QProcess();

  if (!bRemote)
  {
    m_pClientProcess->start(QString::fromUtf8(sPath.GetData()), arguments, QIODevice::OpenModeFlag::NotOpen);

    if (!m_pClientProcess->waitForStarted())
    {
      delete m_pClientProcess;
      m_pClientProcess = nullptr;

      m_pProtocol.Clear();
      m_pChannel.Clear();

      ezLog::Error("Failed to start process '{0}'", sPath);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

bool ezEditorProcessCommunicationChannel::IsClientAlive() const
{
  if (m_pClientProcess == nullptr)
    return false;

  bool bRunning = m_pClientProcess->state() != QProcess::NotRunning;
  bool bNoError = m_pClientProcess->error() == QProcess::UnknownError;

  return bRunning && bNoError;
}

void ezEditorProcessCommunicationChannel::CloseConnection()
{
  if (m_pProtocol)
  {
    m_pProtocol->m_MessageEvent.RemoveEventHandler(ezMakeDelegate(&ezProcessCommunicationChannel::MessageFunc, this));
    m_pProtocol.Clear();
  }
  m_pChannel.Clear();

  if (m_pClientProcess)
  {
    m_pClientProcess->close();
    delete m_pClientProcess;
    m_pClientProcess = nullptr;
  }
}

ezString ezEditorProcessCommunicationChannel::GetStdoutContents()
{
  if (m_pClientProcess)
  {
    QByteArray output = m_pClientProcess->readAllStandardOutput();
    return ezString(ezStringView((const char*)output.data(), output.size()));
  }
  return ezString();
}

//////////////////////////////////////////////////////////////////////////

ezResult ezEditorProcessRemoteCommunicationChannel::ConnectToServer(const char* szAddress)
{
  EZ_LOG_BLOCK("ezEditorProcessRemoteCommunicationChannel::ConnectToServer");

  EZ_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");

  m_pFirstAllowedMessageType = nullptr;

  m_pChannel = ezIpcChannel::CreateNetworkChannel(szAddress, ezIpcChannel::Mode::Client);
  m_pProtocol = EZ_DEFAULT_NEW(ezIpcProcessMessageProtocol, m_pChannel.Borrow());
  m_pProtocol->m_MessageEvent.AddEventHandler(ezMakeDelegate(&ezProcessCommunicationChannel::MessageFunc, this));
  m_pChannel->Connect();

  return EZ_SUCCESS;
}

bool ezEditorProcessRemoteCommunicationChannel::IsConnected() const
{
  return m_pChannel->IsConnected();
}

void ezEditorProcessRemoteCommunicationChannel::CloseConnection()
{
  if (m_pProtocol)
  {
    m_pProtocol->m_MessageEvent.RemoveEventHandler(ezMakeDelegate(&ezProcessCommunicationChannel::MessageFunc, this));
    m_pProtocol.Clear();
  }
  m_pChannel.Clear();
}

void ezEditorProcessRemoteCommunicationChannel::TryConnect()
{
  if (m_pChannel && !m_pChannel->IsConnected())
  {
    m_pChannel->Connect();
  }
}
