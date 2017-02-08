#include <Fileserver/Main.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <FileservePlugin/Network/NetworkInterfaceEnet.h>
#include <FileservePlugin/Client/FileserveDataDir.h>
#include <FileservePlugin/Fileserver/Fileserver.h>

ezFileserverApp::ezFileserverApp()
{
}

void ezFileserverApp::AfterCoreStartup()
{
  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  // Add standard folder factory
  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

  // Add the empty data directory to access files via absolute paths
  ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites);

  EZ_DEFAULT_NEW(ezFileserver);

  ezFileserver::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezFileserverApp::FileserverEventHandler, this));
  ezFileserver::GetSingleton()->StartServer();
}

void ezFileserverApp::BeforeCoreShutdown()
{
  ezFileserver::GetSingleton()->StopServer();
  ezFileserver::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezFileserverApp::FileserverEventHandler, this));

  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
}

ezApplication::ApplicationExecution ezFileserverApp::Run()
{
  if (ezFileserver::GetSingleton()->UpdateServer() == false)
  {
    m_uiSleepCounter++;

    if (m_uiSleepCounter > 1000)
    {
      // only sleep when no work had to be done in a while
      ezThreadUtils::Sleep(10);
    }
    else if (m_uiSleepCounter > 10)
    {
      // only sleep when no work had to be done in a while
      ezThreadUtils::Sleep(1);
    }
  }
  else
  {
    m_uiSleepCounter = 0;
  }

  return ezApplication::Continue;
}

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
  }
}

EZ_CONSOLEAPP_ENTRY_POINT(ezFileserverApp);
