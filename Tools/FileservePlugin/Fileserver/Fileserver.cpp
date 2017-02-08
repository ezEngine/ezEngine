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
}

void ezFileserver::StopServer()
{
  if (!m_Network)
    return;

  m_Network->ShutdownConnection();
  m_Network.Reset();
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
  auto& client = m_Clients[msg.GetApplicationID()];

  if (client.m_uiApplicationID != msg.GetApplicationID())
  {
    client.m_uiApplicationID = msg.GetApplicationID();
    ezLog::Info("Connected to new client with ID {0}", msg.GetApplicationID());
  }

  if (msg.GetMessageID() == 'MNT')
  {
    HandleMountRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'READ')
  {
    HandleFileRequest(client, msg);
    return;
  }

  ezLog::Error("Unknown FSRV message: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageSize());
}

void ezFileserver::HandleMountRequest(ezFileserveClientContext& client, ezNetworkMessage &msg)
{
  ezStringBuilder sDataDir, sRootName, sMountPoint;
  ezUInt16 uiDataDirID = 0xffff;

  msg.GetReader() >> sDataDir;
  msg.GetReader() >> sRootName;
  msg.GetReader() >> sMountPoint;
  msg.GetReader() >> uiDataDirID;

  ezLog::Info(" Mounting: '{0}', RootName = '{1}', MountPoint = '{2}', ID = {3}", sDataDir, sRootName, sMountPoint, uiDataDirID);

  EZ_ASSERT_DEV(uiDataDirID >= client.m_MountedDataDirs.GetCount(), "Data dir ID should be larger than previous IDs");

  client.m_MountedDataDirs.SetCount(ezMath::Max<ezUInt32>(uiDataDirID + 1, client.m_MountedDataDirs.GetCount()));
  auto& dir = client.m_MountedDataDirs[uiDataDirID];
  dir.m_sPathOnClient = sDataDir;
  dir.m_sPathOnServer = sDataDir; /// \todo Redirect path
  dir.m_sRootName = sRootName;
  dir.m_sMountPoint = sMountPoint;
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

  const ezFileserveFileState filestate = client.GetFileStatus(uiDataDirID, sRequestedFile, status, m_Upload);

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
}

