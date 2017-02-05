#include <PCH.h>
#include <FileservePlugin/Client/FileserveClient.h>
#include <Foundation/Logging/Log.h>
#include <FileservePlugin/Network/NetworkInterfaceEnet.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

EZ_IMPLEMENT_SINGLETON(ezFileserveClient);

ezFileserveClient::ezFileserveClient()
  : m_SingletonRegistrar(this)
{
}

ezFileserveClient::~ezFileserveClient()
{
  ezLog::Info("Shutting down fileserve client");

  if (m_Network)
  {
    m_Network->ShutdownConnection();
    m_Network.Reset();
  }
}

void ezFileserveClient::EnsureConnected()
{
  if (m_Network == nullptr)
  {
    m_sFileserveCacheFolder = ezOSFile::GetUserDataFolder("FileserveCache");

    if (ezOSFile::CreateDirectoryStructure(m_sFileserveCacheFolder).Failed())
      ezLog::Error("Could not create fileserve cache folder '{0}'", m_sFileserveCacheFolder);

    {
      ezStringBuilder dummy(m_sFileserveCacheFolder, "/Dummy.txt");
      ezOSFile file;
      if (file.Open(dummy, ezFileMode::Write).Succeeded())
      {
        file.Close();
      }
      else
      {
        ezLog::Error("Could not write to dummy file");
      }
    }

    m_Network = EZ_DEFAULT_NEW(ezNetworkInterfaceEnet);
    m_Network->ConnectToServer('EZFS', "localhost:1042");

    ezUInt32 uiMaxRounds = 100;
    while (!m_Network->IsConnectedToServer() && uiMaxRounds > 0)
    {
      --uiMaxRounds;
      m_Network->UpdateNetwork();
      ezThreadUtils::Sleep(100);
    }

    if (uiMaxRounds == 0)
      ezLog::Error("Connection to ezFileserver timed out");
    else
    {
      ezLog::Info("Connected to ezFileserver");

      m_Network->SetMessageHandler('FSRV', ezMakeDelegate(&ezFileserveClient::NetworkMsgHandler, this));
    }
  }
}

void ezFileserveClient::UpdateClient()
{
  if (!m_Network->IsConnectedToServer())
    return;

  m_Network->ExecuteAllMessageHandlers();
}

void ezFileserveClient::MountDataDirectory(const char* szDataDirectory)
{
  if (!m_Network->IsConnectedToServer())
    return;

  ezNetworkMessage msg('FSRV', 'MNT');
  msg.GetWriter() << szDataDirectory;

  m_Network->Send(ezNetworkTransmitMode::Reliable, msg);
}

void GetFileserveCachePath(const char* szFile, ezStringBuilder& sNewPath, const char* szCacheFolder);

void ezFileserveClient::NetworkMsgHandler(ezNetworkMessage& msg)
{
  if (msg.GetMessageID() == 'FILE')
  {
    ezUInt16 uiFileID = 0;
    msg.GetReader() >> uiFileID;

    if (uiFileID == m_uiCurFileDownload)
    {
      ezUInt16 uiChunkSize = 0;
      msg.GetReader() >> uiChunkSize;

      ezUInt32 uiStartPos = m_Download.GetCount();
      m_Download.SetCountUninitialized(uiStartPos + uiChunkSize);
      msg.GetReader().ReadBytes(&m_Download[uiStartPos], uiChunkSize);

      ezLog::Warning("Receiving {0} bytes of download {1} ({2})", uiChunkSize, uiFileID, m_sCurrentFileDownload);
    }

    if (uiFileID == 0) // download finished
    {
      ezLog::Warning("Finished download: {0} bytes for {1}", m_Download.GetCount(), m_sCurrentFileDownload);

      if (!m_Download.IsEmpty())
      {
        ezStringBuilder sCachedFile;
        GetFileserveCachePath(m_sCurrentFileDownload, sCachedFile, m_sFileserveCacheFolder);

        // write the downloaded file to our cache directory
        ezOSFile file;
        if (file.Open(sCachedFile, ezFileMode::Write).Succeeded())
        {

          file.Write(m_Download.GetData(), m_Download.GetCount());

          file.Close();

          ezLog::Success("Wrote cache file to '{0}'", sCachedFile);
        }
        else
        {
          ezLog::Error("Failed to write download to '{0}'", sCachedFile);
        }
      }
      else
        ezLog::Error("File could not be downloaded: '{0}'", m_sCurrentFileDownload);

      m_Download.Clear();
      m_bDownloading = false;
      return;
    }

  }

  ezLog::Info("FSRV: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageSize());
}


void ezFileserveClient::DownloadFile(const char* szFile)
{
  if (!m_Network->IsConnectedToServer())
    return;

  if (m_bDownloading)
    return;

  m_Download.Clear();

  m_sCurrentFileDownload = szFile;
  m_bDownloading = true;
  ++m_uiCurFileDownload;

  ezNetworkMessage msg('FSRV', 'READ');
  msg.GetWriter() << szFile;
  msg.GetWriter() << m_uiCurFileDownload;

  m_Network->Send(ezNetworkTransmitMode::Reliable, msg);
}

EZ_ON_GLOBAL_EVENT(GameApp_UpdatePlugins)
{
  if (ezFileserveClient::GetSingleton())
  {
    ezFileserveClient::GetSingleton()->UpdateClient();
  }
}
