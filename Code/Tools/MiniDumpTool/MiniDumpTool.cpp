#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/Utilities/CommandLineOptions.h>

ezCommandLineOptionInt opt_PID("_MiniDumpTool", "-PID", "Process ID of the application for which to create a crash dump.", 0);

ezCommandLineOptionPath opt_DumpFile("_MiniDumpTool", "-f", "Path to the crash dump file to write.", "");

class ezMiniDumpTool : public ezApplication
{
  ezUInt32 m_uiProcessID = 0;
  ezStringBuilder m_sDumpFile;

public:
  using SUPER = ezApplication;

  ezMiniDumpTool()
    : ezApplication("MiniDumpTool")
  {
  }

  ezResult ParseArguments()
  {
    ezCommandLineUtils* cmd = ezCommandLineUtils::GetGlobalInstance();

    m_uiProcessID = cmd->GetUIntOption("-PID");

    m_sDumpFile = opt_DumpFile.GetOptionValue(ezCommandLineOption::LogMode::Always);
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
    // Add the empty data directory to access files via absolute paths
    ezFileSystem::AddDataDirectory("", "App", ":", ezDataDirUsage::AllowWrites).IgnoreResult();

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

  virtual Execution Run() override
  {
    {
      ezStringBuilder cmdHelp;
      if (ezCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, ezCommandLineOption::LogAvailableModes::IfHelpRequested, "_MiniDumpTool"))
      {
        ezLog::Print(cmdHelp);
        return ezApplication::Execution::Quit;
      }
    }

    if (ParseArguments().Failed())
    {
      SetReturnCode(1);
      return ezApplication::Execution::Quit;
    }

    ezMiniDumpUtils::WriteExternalProcessMiniDump(m_sDumpFile, m_uiProcessID).IgnoreResult();
    return ezApplication::Execution::Quit;
  }
};

EZ_APPLICATION_ENTRY_POINT(ezMiniDumpTool);
