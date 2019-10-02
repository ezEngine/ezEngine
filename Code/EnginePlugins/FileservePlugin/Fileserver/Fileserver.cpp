#include <FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveClient.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Communication/RemoteInterfaceEnet.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/CommandLineUtils.h>

EZ_IMPLEMENT_SINGLETON(ezFileserver);

ezFileserver::ezFileserver()
    : m_SingletonRegistrar(this)
{
  // once a server exists, the client should stay inactive
  ezFileserveClient::DisabledFileserveClient();

  // check whether the fileserve port was reconfigured through the command line
  m_uiPort = ezCommandLineUtils::GetGlobalInstance()->GetIntOption("-fs_port", m_uiPort);
}

void ezFileserver::StartServer()
{
  if (m_Network)
    return;

  ezStringBuilder tmp;

  m_Network = ezRemoteInterfaceEnet::Make();
  m_Network->StartServer('EZFS', ezConversionUtils::ToString(m_uiPort, tmp), false);
  m_Network->SetMessageHandler('FSRV', ezMakeDelegate(&ezFileserver::NetworkMsgHandler, this));
  m_Network->m_RemoteEvents.AddEventHandler(ezMakeDelegate(&ezFileserver::NetworkEventHandler, this));

  ezFileserverEvent e;
  e.m_Type = ezFileserverEvent::Type::ServerStarted;
  m_Events.Broadcast(e);
}

void ezFileserver::StopServer()
{
  if (!m_Network)
    return;

  m_Network->ShutdownConnection();
  m_Network.Clear();

  ezFileserverEvent e;
  e.m_Type = ezFileserverEvent::Type::ServerStopped;
  m_Events.Broadcast(e);
}

bool ezFileserver::UpdateServer()
{
  if (!m_Network)
    return false;

  m_Network->UpdateRemoteInterface();
  return m_Network->ExecuteAllMessageHandlers() > 0;
}

bool ezFileserver::IsServerRunning() const
{
  return m_Network != nullptr;
}

void ezFileserver::SetPort(ezUInt16 uiPort)
{
  EZ_ASSERT_DEV(m_Network == nullptr, "The port cannot be changed after the server was started");
  m_uiPort = uiPort;
}


void ezFileserver::BroadcastReloadResourcesCommand()
{
  if (!IsServerRunning())
    return;

  m_Network->Send('FSRV', 'RLDR');
}

void ezFileserver::NetworkMsgHandler(ezRemoteMessage& msg)
{
  auto& client = DetermineClient(msg);

  if (msg.GetMessageID() == 'HELO')
    return;

  if (msg.GetMessageID() == 'RUTR')
  {
    // 'are you there' is used to check whether a certain address is a proper Fileserver
    m_Network->Send('FSRV', ' YES');

    ezFileserverEvent e;
    e.m_Type = ezFileserverEvent::Type::AreYouThereRequest;
    m_Events.Broadcast(e);
    return;
  }

  if (msg.GetMessageID() == 'READ')
  {
    HandleFileRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UPLH')
  {
    HandleUploadFileHeader(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UPLD')
  {
    HandleUploadFileTransfer(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UPLF')
  {
    HandleUploadFileFinished(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'DELF')
  {
    HandleDeleteFileRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == ' MNT')
  {
    HandleMountRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UMNT')
  {
    HandleUnmountRequest(client, msg);
    return;
  }

  ezLog::Error("Unknown FSRV message: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageSize());
}


void ezFileserver::NetworkEventHandler(const ezRemoteEvent& e)
{
  switch (e.m_Type)
  {
    case ezRemoteEvent::DisconnectedFromClient:
    {
      if (m_Clients.Contains(e.m_uiOtherAppID))
      {
        ezFileserverEvent se;
        se.m_Type = ezFileserverEvent::Type::ClientDisconnected;
        se.m_uiClientID = e.m_uiOtherAppID;

        m_Events.Broadcast(se);

        m_Clients[e.m_uiOtherAppID].m_bLostConnection = true;
      }
    }
    break;

    default:
      break;
  }
}

ezFileserveClientContext& ezFileserver::DetermineClient(ezRemoteMessage& msg)
{
  ezFileserveClientContext& client = m_Clients[msg.GetApplicationID()];

  if (client.m_uiApplicationID != msg.GetApplicationID())
  {
    client.m_uiApplicationID = msg.GetApplicationID();

    ezFileserverEvent e;
    e.m_Type = ezFileserverEvent::Type::ClientConnected;
    e.m_uiClientID = client.m_uiApplicationID;
    m_Events.Broadcast(e);
  }
  else if (client.m_bLostConnection)
  {
    client.m_bLostConnection = false;

    ezFileserverEvent e;
    e.m_Type = ezFileserverEvent::Type::ClientReconnected;
    e.m_uiClientID = client.m_uiApplicationID;
    m_Events.Broadcast(e);
  }

  return client;
}

void ezFileserver::HandleMountRequest(ezFileserveClientContext& client, ezRemoteMessage& msg)
{
  ezStringBuilder sDataDir, sRootName, sMountPoint, sRedir;
  ezUInt16 uiDataDirID = 0xffff;

  msg.GetReader() >> sDataDir;
  msg.GetReader() >> sRootName;
  msg.GetReader() >> sMountPoint;
  msg.GetReader() >> uiDataDirID;

  EZ_ASSERT_DEV(uiDataDirID >= client.m_MountedDataDirs.GetCount(), "Data dir ID should be larger than previous IDs");

  client.m_MountedDataDirs.SetCount(ezMath::Max<ezUInt32>(uiDataDirID + 1, client.m_MountedDataDirs.GetCount()));
  auto& dir = client.m_MountedDataDirs[uiDataDirID];
  dir.m_sPathOnClient = sDataDir;
  dir.m_sRootName = sRootName;
  dir.m_sMountPoint = sMountPoint;

  ezFileserverEvent e;

  if (ezFileSystem::ResolveSpecialDirectory(sDataDir, sRedir).Succeeded())
  {
    dir.m_bMounted = true;
    dir.m_sPathOnServer = sRedir;
    e.m_Type = ezFileserverEvent::Type::MountDataDir;
  }
  else
  {
    dir.m_bMounted = false;
    e.m_Type = ezFileserverEvent::Type::MountDataDirFailed;
  }

  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szName = sRootName;
  e.m_szPath = sDataDir;
  e.m_szRedirectedPath = sRedir;
  m_Events.Broadcast(e);
}


void ezFileserver::HandleUnmountRequest(ezFileserveClientContext& client, ezRemoteMessage& msg)
{
  ezUInt16 uiDataDirID = 0xffff;
  msg.GetReader() >> uiDataDirID;

  EZ_ASSERT_DEV(uiDataDirID < client.m_MountedDataDirs.GetCount(), "Invalid data dir ID to unmount");

  auto& dir = client.m_MountedDataDirs[uiDataDirID];
  dir.m_bMounted = false;

  ezFileserverEvent e;
  e.m_Type = ezFileserverEvent::Type::UnmountDataDir;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = dir.m_sPathOnClient;
  e.m_szName = dir.m_sRootName;
  m_Events.Broadcast(e);
}

void ezFileserver::HandleFileRequest(ezFileserveClientContext& client, ezRemoteMessage& msg)
{
  ezUInt16 uiDataDirID = 0;
  bool bForceThisDataDir = false;

  msg.GetReader() >> uiDataDirID;
  msg.GetReader() >> bForceThisDataDir;

  ezStringBuilder sRequestedFile;
  msg.GetReader() >> sRequestedFile;

  ezUuid downloadGuid;
  msg.GetReader() >> downloadGuid;

  ezFileserveClientContext::FileStatus status;
  msg.GetReader() >> status.m_iTimestamp;
  msg.GetReader() >> status.m_uiHash;

  ezFileserverEvent e;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = sRequestedFile;
  e.m_uiSentTotal = 0;

  const ezFileserveFileState filestate = client.GetFileStatus(uiDataDirID, sRequestedFile, status, m_SendToClient, bForceThisDataDir);

  {
    e.m_Type = ezFileserverEvent::Type::FileDownloadRequest;
    e.m_uiSizeTotal = m_SendToClient.GetCount();
    e.m_FileState = filestate;
    m_Events.Broadcast(e);
  }

  if (filestate == ezFileserveFileState::Different)
  {
    ezUInt32 uiNextByte = 0;
    const ezUInt32 uiFileSize = m_SendToClient.GetCount();

    // send the file over in multiple packages of 1KB each
    // send at least one package, even for empty files
    do
    {
      const ezUInt16 uiChunkSize = (ezUInt16)ezMath::Min<ezUInt32>(1024, m_SendToClient.GetCount() - uiNextByte);

      ezRemoteMessage ret;
      ret.GetWriter() << downloadGuid;
      ret.GetWriter() << uiChunkSize;
      ret.GetWriter() << uiFileSize;

      if (!m_SendToClient.IsEmpty())
        ret.GetWriter().WriteBytes(&m_SendToClient[uiNextByte], uiChunkSize);

      ret.SetMessageID('FSRV', 'DWNL');
      m_Network->Send(ezRemoteTransmitMode::Reliable, ret);

      uiNextByte += uiChunkSize;

      // reuse previous values
      {
        e.m_Type = ezFileserverEvent::Type::FileDownloading;
        e.m_uiSentTotal = uiNextByte;
        m_Events.Broadcast(e);
      }
    } while (uiNextByte < m_SendToClient.GetCount());
  }

  // final answer to client
  {
    ezRemoteMessage ret('FSRV', 'DWNF');
    ret.GetWriter() << downloadGuid;
    ret.GetWriter() << (ezInt8)filestate;
    ret.GetWriter() << status.m_iTimestamp;
    ret.GetWriter() << status.m_uiHash;
    ret.GetWriter() << uiDataDirID;

    m_Network->Send(ezRemoteTransmitMode::Reliable, ret);
  }

  // reuse previous values
  {
    e.m_Type = ezFileserverEvent::Type::FileDownloadFinished;
    m_Events.Broadcast(e);
  }
}

void ezFileserver::HandleDeleteFileRequest(ezFileserveClientContext& client, ezRemoteMessage& msg)
{
  ezUInt16 uiDataDirID = 0xffff;
  msg.GetReader() >> uiDataDirID;

  ezStringBuilder sFile;
  msg.GetReader() >> sFile;

  EZ_ASSERT_DEV(uiDataDirID < client.m_MountedDataDirs.GetCount(), "Invalid data dir ID to unmount");

  ezFileserverEvent e;
  e.m_Type = ezFileserverEvent::Type::FileDeleteRequest;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = sFile;
  m_Events.Broadcast(e);

  const auto& dd = client.m_MountedDataDirs[uiDataDirID];

  ezStringBuilder sAbsPath;
  sAbsPath = dd.m_sPathOnServer;
  sAbsPath.AppendPath(sFile);

  ezOSFile::DeleteFile(sAbsPath);
}

void ezFileserver::HandleUploadFileHeader(ezFileserveClientContext& client, ezRemoteMessage& msg)
{
  ezUInt16 uiDataDirID = 0;

  msg.GetReader() >> m_FileUploadGuid;
  msg.GetReader() >> m_uiFileUploadSize;
  msg.GetReader() >> uiDataDirID;
  msg.GetReader() >> m_sCurFileUpload;

  m_SentFromClient.Clear();
  m_SentFromClient.Reserve(m_uiFileUploadSize);

  ezFileserverEvent e;
  e.m_Type = ezFileserverEvent::Type::FileUploadRequest;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = m_sCurFileUpload;
  e.m_uiSentTotal = 0;
  e.m_uiSizeTotal = m_uiFileUploadSize;

  m_Events.Broadcast(e);
}

void ezFileserver::HandleUploadFileTransfer(ezFileserveClientContext& client, ezRemoteMessage& msg)
{
  ezUuid transferGuid;
  msg.GetReader() >> transferGuid;

  if (transferGuid != m_FileUploadGuid)
    return;

  ezUInt16 uiChunkSize = 0;
  msg.GetReader() >> uiChunkSize;

  const ezUInt32 uiStartPos = m_SentFromClient.GetCount();
  m_SentFromClient.SetCountUninitialized(uiStartPos + uiChunkSize);
  msg.GetReader().ReadBytes(&m_SentFromClient[uiStartPos], uiChunkSize);

  ezFileserverEvent e;
  e.m_Type = ezFileserverEvent::Type::FileUploading;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = m_sCurFileUpload;
  e.m_uiSentTotal = m_SentFromClient.GetCount();
  e.m_uiSizeTotal = m_uiFileUploadSize;

  m_Events.Broadcast(e);
}

void ezFileserver::HandleUploadFileFinished(ezFileserveClientContext& client, ezRemoteMessage& msg)
{
  ezUuid transferGuid;
  msg.GetReader() >> transferGuid;

  if (transferGuid != m_FileUploadGuid)
    return;

  ezUInt16 uiDataDirID = 0;
  msg.GetReader() >> uiDataDirID;

  ezStringBuilder sFile;
  msg.GetReader() >> sFile;

  ezStringBuilder sOutputFile;
  sOutputFile = client.m_MountedDataDirs[uiDataDirID].m_sPathOnServer;
  sOutputFile.AppendPath(sFile);

  {
    ezOSFile file;
    if (file.Open(sOutputFile, ezFileOpenMode::Write).Failed())
    {
      ezLog::Error("Could not write uploaded file to '{0}'", sOutputFile);
      return;
    }

    if (!m_SentFromClient.IsEmpty())
    {
      file.Write(m_SentFromClient.GetData(), m_SentFromClient.GetCount());
    }
  }

  ezFileserverEvent e;
  e.m_Type = ezFileserverEvent::Type::FileUploadFinished;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath = sFile;
  e.m_uiSentTotal = m_SentFromClient.GetCount();
  e.m_uiSizeTotal = m_SentFromClient.GetCount();

  m_Events.Broadcast(e);

  // send a response when all data has been transmitted
  // this ensures the client side updates the network until all data has been fully transmitted
  m_Network->Send('FSRV', 'UACK');
}


ezResult ezFileserver::SendConnectionInfo(const char* szClientAddress, ezUInt16 uiMyPort, const ezArrayPtr<ezStringBuilder>& MyIPs,
                                          ezTime timeout)
{
  ezStringBuilder sAddress = szClientAddress;
  sAddress.Append(":2042"); // hard-coded port

  ezUniquePtr<ezRemoteInterfaceEnet> network = ezRemoteInterfaceEnet::Make();
  EZ_SUCCEED_OR_RETURN(network->ConnectToServer('EZIP', sAddress, false));

  if (network->WaitForConnectionToServer(timeout).Failed())
  {
    network->ShutdownConnection();
    return EZ_FAILURE;
  }

  const ezUInt8 uiCount = MyIPs.GetCount();

  ezRemoteMessage msg('FSRV', 'MYIP');
  msg.GetWriter() << uiMyPort;
  msg.GetWriter() << uiCount;

  for (const auto& info : MyIPs)
  {
    msg.GetWriter() << info;
  }

  network->Send(ezRemoteTransmitMode::Reliable, msg);

  // make sure the message is out, before we shut down
  for (ezUInt32 i = 0; i < 10; ++i)
  {
    network->UpdateRemoteInterface();
    ezThreadUtils::Sleep(ezTime::Milliseconds(1));
  }

  network->ShutdownConnection();
  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(FileservePlugin, FileservePlugin_Fileserver_Fileserver);
