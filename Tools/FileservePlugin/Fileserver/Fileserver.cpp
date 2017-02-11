#include <PCH.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <FileservePlugin/Network/NetworkInterfaceEnet.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Algorithm/Hashing.h>
#include <FileservePlugin/Client/FileserveClient.h>

EZ_IMPLEMENT_SINGLETON(ezFileserver);

ezFileserver::ezFileserver()
  : m_SingletonRegistrar(this)
{
  // once a server exists, the client should stay inactive
  ezFileserveClient::s_bEnableFileserve = false;
}

void ezFileserver::StartServer(ezUInt16 uiPort /*= 1042*/)
{
  m_Network = EZ_DEFAULT_NEW(ezNetworkInterfaceEnet);
  m_Network->StartServer('EZFS', uiPort, false);
  m_Network->SetMessageHandler('FSRV', ezMakeDelegate(&ezFileserver::NetworkMsgHandler, this));

  ezFileserverEvent e;
  e.m_Type = ezFileserverEvent::Type::ServerStarted;
  m_Events.Broadcast(e);
}

void ezFileserver::StopServer()
{
  if (!m_Network)
    return;

  m_Network->ShutdownConnection();
  m_Network.Reset();

  ezFileserverEvent e;
  e.m_Type = ezFileserverEvent::Type::ServerStopped;
  m_Events.Broadcast(e);
}

bool ezFileserver::UpdateServer()
{
  if (!m_Network)
    return false;

  m_Network->UpdateNetwork();
  return m_Network->ExecuteAllMessageHandlers() > 0;
}

void ezFileserver::NetworkMsgHandler(ezNetworkMessage& msg)
{
  auto& client = DetermineClient(msg);

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

  if (msg.GetMessageID() == 'MNT')
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

ezFileserveClientContext& ezFileserver::DetermineClient(ezNetworkMessage &msg)
{
  ezFileserveClientContext& client = m_Clients[msg.GetApplicationID()];

  if (client.m_uiApplicationID != msg.GetApplicationID())
  {
    client.m_uiApplicationID = msg.GetApplicationID();

    ezFileserverEvent e;
    e.m_Type = ezFileserverEvent::Type::ConnectedNewClient;
    m_Events.Broadcast(e);
  }

  return client;
}

void ezFileserver::HandleMountRequest(ezFileserveClientContext& client, ezNetworkMessage &msg)
{
  ezStringBuilder sDataDir, sRootName, sMountPoint;
  ezUInt16 uiDataDirID = 0xffff;

  msg.GetReader() >> sDataDir;
  msg.GetReader() >> sRootName;
  msg.GetReader() >> sMountPoint;
  msg.GetReader() >> uiDataDirID;

  EZ_ASSERT_DEV(uiDataDirID >= client.m_MountedDataDirs.GetCount(), "Data dir ID should be larger than previous IDs");

  client.m_MountedDataDirs.SetCount(ezMath::Max<ezUInt32>(uiDataDirID + 1, client.m_MountedDataDirs.GetCount()));
  auto& dir = client.m_MountedDataDirs[uiDataDirID];
  dir.m_sPathOnClient = sDataDir;
  dir.m_sPathOnServer = sDataDir; /// \todo Redirect path
  dir.m_sRootName = sRootName;
  dir.m_sMountPoint = sMountPoint;
  dir.m_bMounted = true;

  ezFileserverEvent e;
  e.m_Type = ezFileserverEvent::Type::MountDataDir;
  e.m_szPath = sDataDir;
  e.m_szDataDirRootName = sRootName;
  m_Events.Broadcast(e);
}


void ezFileserver::HandleUnmountRequest(ezFileserveClientContext& client, ezNetworkMessage &msg)
{
  ezUInt16 uiDataDirID = 0xffff;
  msg.GetReader() >> uiDataDirID;

  EZ_ASSERT_DEV(uiDataDirID < client.m_MountedDataDirs.GetCount(), "Invalid data dir ID to unmount");

  auto& dir = client.m_MountedDataDirs[uiDataDirID];
  dir.m_bMounted = false;

  ezFileserverEvent e;
  e.m_Type = ezFileserverEvent::Type::UnmountDataDir;
  e.m_szPath = dir.m_sPathOnClient;
  e.m_szDataDirRootName = dir.m_sRootName;
  m_Events.Broadcast(e);
}

void ezFileserver::HandleFileRequest(ezFileserveClientContext& client, ezNetworkMessage &msg)
{
  ezUInt16 uiDataDirID = 0;

  msg.GetReader() >> uiDataDirID;

  ezStringBuilder sRequestedFile;
  msg.GetReader() >> sRequestedFile;

  ezUuid downloadGuid;
  msg.GetReader() >> downloadGuid;

  ezFileserveClientContext::FileStatus status;
  msg.GetReader() >> status.m_iTimestamp;
  msg.GetReader() >> status.m_uiHash;

  ezFileserverEvent e;
  e.m_szPath = sRequestedFile;
  e.m_uiSentTotal = 0;

  /// \todo Cache the file state on the server side as well
  const ezFileserveFileState filestate = client.GetFileStatus(uiDataDirID, sRequestedFile, status, m_SendToClient);

  {
    e.m_Type = ezFileserverEvent::Type::FileRequest;
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

      ezNetworkMessage ret;
      ret.GetWriter() << downloadGuid;
      ret.GetWriter() << uiChunkSize;
      ret.GetWriter() << uiFileSize;

      if (!m_SendToClient.IsEmpty())
        ret.GetWriter().WriteBytes(&m_SendToClient[uiNextByte], uiChunkSize);

      ret.SetMessageID('FSRV', 'DWNL');
      m_Network->Send(ezNetworkTransmitMode::Reliable, ret);

      uiNextByte += uiChunkSize;

      // reuse previous values
      {
        e.m_Type = ezFileserverEvent::Type::FileTranser;
        e.m_uiSentTotal = uiNextByte;
        m_Events.Broadcast(e);
      }
    }
    while (uiNextByte < m_SendToClient.GetCount());
  }

  // final answer to client
  {
    const ezUInt16 uiEndToken = 0; // chunk size

    ezNetworkMessage ret('FSRV', 'DWNF');
    ret.GetWriter() << downloadGuid;
    ret.GetWriter() << (ezInt8)filestate;
    ret.GetWriter() << status.m_iTimestamp;
    ret.GetWriter() << status.m_uiHash;

    m_Network->Send(ezNetworkTransmitMode::Reliable, ret);
  }

  // reuse previous values
  {
    e.m_Type = ezFileserverEvent::Type::FileTranserFinished;
    m_Events.Broadcast(e);
  }
}

void ezFileserver::HandleDeleteFileRequest(ezFileserveClientContext& client, ezNetworkMessage &msg)
{
  ezUInt16 uiDataDirID = 0xffff;
  msg.GetReader() >> uiDataDirID;

  ezStringBuilder sFile;
  msg.GetReader() >> sFile;

  EZ_ASSERT_DEV(uiDataDirID < client.m_MountedDataDirs.GetCount(), "Invalid data dir ID to unmount");

  ezFileserverEvent e;
  e.m_Type = ezFileserverEvent::Type::FileDeleteRequest;
  e.m_szPath = sFile;
  m_Events.Broadcast(e);

  const auto& dd = client.m_MountedDataDirs[uiDataDirID];

  ezStringBuilder sAbsPath;
  sAbsPath = dd.m_sPathOnServer;
  sAbsPath.AppendPath(sFile);

  ezOSFile::DeleteFile(sAbsPath);
}

void ezFileserver::HandleUploadFileHeader(ezFileserveClientContext& client, ezNetworkMessage &msg)
{
  ezUInt16 uiDataDirID = 0;

  msg.GetReader() >> m_FileUploadGuid;
  msg.GetReader() >> m_uiFileUploadSize;
  msg.GetReader() >> uiDataDirID;
  msg.GetReader() >> m_sCurFileUpload;

  m_SentFromClient.Clear();
  m_SentFromClient.Reserve(m_uiFileUploadSize);
}

void ezFileserver::HandleUploadFileTransfer(ezFileserveClientContext& client, ezNetworkMessage &msg)
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
  e.m_szPath = m_sCurFileUpload;
  e.m_uiSentTotal = m_SentFromClient.GetCount();
  e.m_uiSizeTotal = m_uiFileUploadSize;

  m_Events.Broadcast(e);
}

void ezFileserver::HandleUploadFileFinished(ezFileserveClientContext& client, ezNetworkMessage &msg)
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
    if (file.Open(sOutputFile, ezFileMode::Write).Failed())
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
  e.m_Type = ezFileserverEvent::Type::FileUploadedFinished;
  e.m_szPath = sFile;
  e.m_uiSentTotal = m_SentFromClient.GetCount();
  e.m_uiSizeTotal = m_SentFromClient.GetCount();

  m_Events.Broadcast(e);
}

