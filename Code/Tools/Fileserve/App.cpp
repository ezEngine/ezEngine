#include <FileservePCH.h>

#include <Fileserve/Main.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Utilities/CommandLineUtils.h>

void ezFileserverApp::AfterCoreSystemsStartup()
{
  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  // Add the empty data directory to access files via absolute paths
  ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites);

  EZ_DEFAULT_NEW(ezFileserver);

  ezFileserver::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezFileserverApp::FileserverEventHandler, this));

#ifndef EZ_USE_QT
  ezFileserver::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezFileserverApp::FileserverEventHandlerConsole, this));
  ezFileserver::GetSingleton()->StartServer();
#endif

  m_CloseAppTimeout = ezTime::Seconds(ezCommandLineUtils::GetGlobalInstance()->GetIntOption("-fs_close_timeout", 0));
  m_TimeTillClosing = ezTime::Seconds(ezCommandLineUtils::GetGlobalInstance()->GetIntOption("-fs_wait_timeout", 0));

  if (m_TimeTillClosing.GetSeconds() > 0)
  {
    m_TimeTillClosing += ezTime::Now();
  }
}

void ezFileserverApp::BeforeCoreSystemsShutdown()
{
  ezFileserver::GetSingleton()->StopServer();

#ifndef EZ_USE_QT
  ezFileserver::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezFileserverApp::FileserverEventHandlerConsole, this));
#endif

  ezFileserver::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezFileserverApp::FileserverEventHandler, this));

  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

ezApplication::ApplicationExecution ezFileserverApp::Run()
{
  // if there are no more connections, and we have a timeout to close when no connections are left, we return Quit
  if (m_uiConnections == 0 && m_TimeTillClosing > ezTime::Seconds(0) && ezTime::Now() > m_TimeTillClosing)
  {
    return ezApplication::Quit;
  }

  if (ezFileserver::GetSingleton()->UpdateServer() == false)
  {
    m_uiSleepCounter++;

    if (m_uiSleepCounter > 1000)
    {
      // only sleep when no work had to be done in a while
      ezThreadUtils::Sleep(ezTime::Milliseconds(10));
    }
    else if (m_uiSleepCounter > 10)
    {
      // only sleep when no work had to be done in a while
      ezThreadUtils::Sleep(ezTime::Milliseconds(1));
    }
  }
  else
  {
    m_uiSleepCounter = 0;
  }

  return ezApplication::Continue;
}
