#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/MiniDumpUtils.h>

class ezMiniDumpTool : public ezApplication
{
  ezUInt32 m_uiProcessID = 0;
  ezStringBuilder m_sDumpFile;

public:
  typedef ezApplication SUPER;

  ezMiniDumpTool()
    : ezApplication("MiniDumpTool")
  {
  }

  ezResult ParseArguments()
  {
    ezCommandLineUtils* cmd = ezCommandLineUtils::GetGlobalInstance();

    m_uiProcessID = cmd->GetUIntOption("-PID");
    m_sDumpFile = cmd->GetStringOption("-f", 0, m_sDumpFile);
    m_sDumpFile.MakeCleanPath();

    if (m_uiProcessID == 0)
    {
      ezLog::Error("Missing '-PID' argument");
      return EZ_FAILURE;
    }


    return EZ_SUCCESS;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    // Add standard folder factory
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    // Add the empty data directory to access files via absolute paths
    ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites);

    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    // prevent further output during shutdown
    ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

    SUPER::BeforeCoreSystemsShutdown();
  }

  virtual ApplicationExecution Run() override
  {
    if (ParseArguments().Failed())
    {
      SetReturnCode(1);
      return ezApplication::Quit;
    }

    ezMiniDumpUtils::WriteExternalProcessMiniDump(m_sDumpFile, m_uiProcessID);
    return ezApplication::Quit;
  }
};

EZ_CONSOLEAPP_ENTRY_POINT(ezMiniDumpTool);
