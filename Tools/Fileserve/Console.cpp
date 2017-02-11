#include <Fileserve/Main.h>
#include <Foundation/Logging/Log.h>

void ezFileserverApp::FileserverEventHandler(const ezFileserverEvent& e)
{
  switch (e.m_Type)
  {
  case ezFileserverEvent::Type::None:
    ezLog::Error("Invalid Fileserver event type");
    break;

  case ezFileserverEvent::Type::ServerStarted:
    {
      ezLog::Info("ezFileserver is running");
    }
    break;

  case ezFileserverEvent::Type::ServerStopped:
    {
      ezLog::Info("ezFileserver was shut down");
    }
    break;

  case ezFileserverEvent::Type::ConnectedNewClient:
    {
      ezLog::Success("Client connected");
    }
    break;

  case ezFileserverEvent::Type::MountDataDir:
    {
      ezLog::Info("Mounted data directory '{0}' ({1})", e.m_szDataDirRootName, e.m_szPath);
    }
    break;

  case ezFileserverEvent::Type::UnmountDataDir:
    {
      ezLog::Info("Unmount request for data directory '{0}' ({1})", e.m_szDataDirRootName, e.m_szPath);
    }
    break;

  case ezFileserverEvent::Type::FileRequest:
    {
      if (e.m_FileState == ezFileserveFileState::NonExistant)
        ezLog::Dev("Request: (N/A) '{0}'", e.m_szPath);

      if (e.m_FileState == ezFileserveFileState::SameHash)
        ezLog::Dev("Request: (HASH) '{0}'", e.m_szPath);

      if (e.m_FileState == ezFileserveFileState::SameTimestamp)
        ezLog::Dev("Request: (TIME) '{0}'", e.m_szPath);

      if (e.m_FileState == ezFileserveFileState::NonExistantEither)
        ezLog::Dev("Request: (N/AE) '{0}'", e.m_szPath);

      if (e.m_FileState == ezFileserveFileState::Different)
        ezLog::Info("Request: '{0}' ({1} bytes)", e.m_szPath, e.m_uiSizeTotal);
    }
    break;

  case ezFileserverEvent::Type::FileTranser:
    {
      ezLog::Debug("Transfer: {0}/{1} bytes", e.m_uiSentTotal, e.m_uiSizeTotal, e.m_szPath);
    }
    break;

  case ezFileserverEvent::Type::FileTranserFinished:
    {
      if (e.m_FileState == ezFileserveFileState::Different)
        ezLog::Info("Transfer done.");
    }
    break;

  case ezFileserverEvent::Type::FileDeleteRequest:
    {
      ezLog::Warning("File Deletion: '{0}'", e.m_szPath);
    }
    break;

  case ezFileserverEvent::Type::FileUploading:
    ezLog::Debug("Upload: {0}/{1} bytes", e.m_uiSentTotal, e.m_uiSizeTotal, e.m_szPath);
    break;

  case ezFileserverEvent::Type::FileUploadedFinished:
    ezLog::Info("Upload finished: {0}", e.m_szPath);
    break;
  }
}
