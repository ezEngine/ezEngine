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

  ezUInt16 uiDownloadID;
  msg.GetReader() >> uiDownloadID;

  ezFileserveClientContext::FileStatus status;
  msg.GetReader() >> status.m_iTimestamp;
  msg.GetReader() >> status.m_uiHash;

  ezFileserverEvent e;
  e.m_szPath = sRequestedFile;
  e.m_uiSentTotal = 0;

  /// \todo Cache the file state on the server side as well
  const ezFileserveFileState filestate = client.GetFileStatus(uiDataDirID, sRequestedFile, status, m_Upload);

  {
    e.m_Type = ezFileserverEvent::Type::FileRequest;
    e.m_uiSizeTotal = m_Upload.GetCount();
    e.m_FileState = filestate;
    m_Events.Broadcast(e);
  }

  if (filestate == ezFileserveFileState::Different)
  {
    ezUInt32 uiNextByte = 0;
    const ezUInt32 uiFileSize = m_Upload.GetCount();

    // send the file over in multiple packages of 1KB each
    // send at least one package, even for empty files
    do
    {
      const ezUInt16 uiChunkSize = (ezUInt16)ezMath::Min<ezUInt32>(1024, m_Upload.GetCount() - uiNextByte);

      ezNetworkMessage ret;
      ret.GetWriter() << uiDownloadID;
      ret.GetWriter() << uiChunkSize;
      ret.GetWriter() << uiFileSize;

      if (!m_Upload.IsEmpty())
        ret.GetWriter().WriteBytes(&m_Upload[uiNextByte], uiChunkSize);

      ret.SetMessageID('FSRV', 'FILE');
      m_Network->Send(ezNetworkTransmitMode::Reliable, ret);

      uiNextByte += uiChunkSize;

      // reuse previous values
      {
        e.m_Type = ezFileserverEvent::Type::FileTranser;
        e.m_uiSentTotal = uiNextByte;
        m_Events.Broadcast(e);
      }
    }
    while (uiNextByte < m_Upload.GetCount());
  }

  // final answer to client
  {
    const ezUInt16 uiEndToken = 0; // chunk size

    ezNetworkMessage ret('FSRV', 'FINE');
    ret.GetWriter() << uiDownloadID;
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

