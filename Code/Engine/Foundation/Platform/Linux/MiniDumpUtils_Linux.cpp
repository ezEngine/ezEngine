#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX)

#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/System/MiniDumpUtils.h>
#  include <Foundation/System/Process.h>
#  include <Foundation/System/ProcessGroup.h>
#  include <Foundation/Utilities/CommandLineOptions.h>

#  include <sys/wait.h>
#  include <unistd.h>

ezCommandLineOptionBool opt_FullCrashDumps("app", "-fullcrashdumps", "If enabled, crash dumps will contain the full memory image.", false);

ezStatus ezMiniDumpUtils::WriteExternalProcessMiniDump(ezStringView sDumpFile, ezUInt32 uiProcessID, ezDumpType dumpTypeOverride)
{
  // Create the output directory if needed
  {
    ezStringBuilder folder = sDumpFile;
    folder.PathParentDirectory();
    if (ezOSFile::CreateDirectoryStructure(folder).Failed())
      return ezStatus("Failed to create output directory structure.");
  }

  // gcore outputs to <dumpfile>.<pid> format, so we need to handle the filename
  ezStringBuilder sCoreName = sDumpFile;
  sCoreName.RemoveFileExtension();

  // Run gcore to generate the core dump
  // gcore -o <output_prefix> <pid>
  ezProcessOptions procOpt;
  procOpt.m_bHideConsoleWindow = true;
  procOpt.m_sProcess = "gcore";
  procOpt.m_Arguments.PushBack("-o");
  procOpt.m_Arguments.PushBack(sCoreName);
  procOpt.AddArgument("{}", uiProcessID);

  ezInt32 iExitCode = -1;
  if (ezProcess::Execute(procOpt, &iExitCode).Failed())
  {
    return ezStatus("gcore not found. Install gdb package to enable crash dump support.");
  }

  if (iExitCode != 0)
  {
    return ezStatus(ezFmt("gcore failed with exit code {}", iExitCode));
  }

  // gcore creates file as <prefix>.<pid>, rename to requested name
  ezStringBuilder sGcoreOutput;
  sGcoreOutput.SetFormat("{}.{}", sCoreName, uiProcessID);

  if (ezOSFile::ExistsFile(sGcoreOutput))
  {
    if (sGcoreOutput != sDumpFile)
    {
      ezOSFile::MoveFileOrDirectory(sGcoreOutput, sDumpFile).IgnoreResult();
    }
  }

  return EZ_SUCCESS;
}

ezStatus ezMiniDumpUtils::WriteOwnProcessMiniDump(ezStringView sDumpFile, void* pOsSpecificData, ezDumpType dumpTypeOverride)
{
  // Writing a dump of our own process is tricky because we're potentially in a crashed state.
  // The preferred approach is LaunchMiniDumpTool. This function is a fallback.

  // Create the output directory if needed
  {
    ezStringBuilder folder = sDumpFile;
    folder.PathParentDirectory();
    if (ezOSFile::CreateDirectoryStructure(folder).Failed())
      return ezStatus("Failed to create output directory structure.");
  }

  pid_t myPid = getpid();

  // Fork a child to run gcore on us
  pid_t childPid = fork();
  if (childPid == 0)
  {
    // Child process - run gcore on parent
    ezStringBuilder sCoreName = sDumpFile;
    sCoreName.RemoveFileExtension();

    ezStringBuilder sPidArg;
    sPidArg.SetFormat("{}", myPid);

    execlp("gcore", "gcore", "-o", sCoreName.GetData(), sPidArg.GetData(), nullptr);
    _exit(1); // execlp failed
  }
  else if (childPid > 0)
  {
    // Parent - wait for child
    int status;
    waitpid(childPid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    {
      // gcore succeeded, rename file from <prefix>.<pid> to requested name
      ezStringBuilder sCoreName = sDumpFile;
      sCoreName.RemoveFileExtension();

      ezStringBuilder sGcoreOutput;
      sGcoreOutput.SetFormat("{}.{}", sCoreName, myPid);

      if (ezOSFile::ExistsFile(sGcoreOutput))
      {
        if (sGcoreOutput != sDumpFile)
        {
          ezOSFile::MoveFileOrDirectory(sGcoreOutput, sDumpFile).IgnoreResult();
        }
        return EZ_SUCCESS;
      }
    }
  }

  return ezStatus("Could not write mini dump - gcore not available or failed.");
}

ezStatus ezMiniDumpUtils::LaunchMiniDumpTool(ezStringView sDumpFile, ezDumpType dumpTypeOverride)
{
  ezStringBuilder sDumpToolPath = ezOSFile::GetApplicationDirectory();
  sDumpToolPath.AppendPath("ezMiniDumpTool");
  sDumpToolPath.MakeCleanPath();

  if (!ezOSFile::ExistsFile(sDumpToolPath))
    return ezStatus(ezFmt("ezMiniDumpTool not found in '{}'", sDumpToolPath));

  ezProcessOptions procOpt;
  procOpt.m_bHideConsoleWindow = true;
  procOpt.m_sProcess = sDumpToolPath;
  procOpt.m_Arguments.PushBack("-PID");
  procOpt.AddArgument("{}", ezProcess::GetCurrentProcessID());
  procOpt.m_Arguments.PushBack("-f");
  procOpt.m_Arguments.PushBack(sDumpFile);

  if ((opt_FullCrashDumps.GetOptionValue(ezCommandLineOption::LogMode::Always) && dumpTypeOverride == ezDumpType::Auto) || dumpTypeOverride == ezDumpType::MiniDumpWithFullMemory)
  {
    // Forward the '-fullcrashdumps' command line argument
    procOpt.AddArgument("-fullcrashdumps");
  }

  ezProcessGroup proc;
  if (proc.Launch(procOpt).Failed())
    return ezStatus(ezFmt("Failed to launch '{}'", sDumpToolPath));

  if (proc.WaitToFinish().Failed())
    return ezStatus("Waiting for ezMiniDumpTool to finish failed.");

  return EZ_SUCCESS;
}

#endif
