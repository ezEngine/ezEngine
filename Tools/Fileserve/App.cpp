#include <Fileserve/Main.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>

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

