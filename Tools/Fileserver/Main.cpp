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
  ezDataDirectory::FileserveType::s_bEnableFileserve = false;

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  // Add standard folder factory
  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

  // Add the empty data directory to access files via absolute paths
  ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites);

  EZ_DEFAULT_NEW(ezFileserver);

  ezFileserver::GetSingleton()->StartServer();
}

void ezFileserverApp::BeforeCoreShutdown()
{
  ezFileserver::GetSingleton()->StopServer();

  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
}

ezApplication::ApplicationExecution ezFileserverApp::Run()
{
  ezFileserver::GetSingleton()->UpdateServer();

  ezThreadUtils::Sleep(25);
  return ezApplication::Continue;
}

EZ_CONSOLEAPP_ENTRY_POINT(ezFileserverApp);
