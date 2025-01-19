#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Platform/Win/DosDevicePath_Win.h>
#  include <Foundation/Platform/Win/Utils/MinWindows.h>
#  include <Foundation/System/MiniDumpUtils.h>
#  include <Foundation/System/ProcessGroup.h>
#  include <Foundation/Types/ScopeExit.h>
#  include <Foundation/Utilities/CommandLineOptions.h>
#  include <Foundation/Utilities/CommandLineUtils.h>

#  include <Dbghelp.h>
#  include <Shlwapi.h>
#  include <tchar.h>
#  include <werapi.h>

ezCommandLineOptionBool opt_FullCrashDumps("app", "-fullcrashdumps", "If enabled, crash dumps will contain the full memory image.", false);

using MINIDUMPWRITEDUMP = BOOL(WINAPI*)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
  PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

ezMinWindows::HANDLE ezMiniDumpUtils::GetProcessHandleWithNecessaryRights(ezUInt32 uiProcessID)
{
  // try to get more than we need
  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, uiProcessID);

  if (hProcess == NULL)
  {
    // try to get all that we need for a nice dump
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE, FALSE, uiProcessID);
  }

  if (hProcess == NULL)
  {
    // try to get rights for a limited dump
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, uiProcessID);
  }

  return hProcess;
}

ezStatus ezMiniDumpUtils::WriteProcessMiniDump(ezStringView sDumpFile, ezUInt32 uiProcessID, ezMinWindows::HANDLE hProcess, struct _EXCEPTION_POINTERS* pExceptionInfo, ezDumpType dumpTypeOverride)
{
  HMODULE hDLL = ::LoadLibraryA("dbghelp.dll");

  if (hDLL == nullptr)
  {
    return ezStatus("dbghelp.dll could not be loaded.");
  }

  MINIDUMPWRITEDUMP MiniDumpWriteDumpFunc = (MINIDUMPWRITEDUMP)::GetProcAddress(hDLL, "MiniDumpWriteDump");

  if (MiniDumpWriteDumpFunc == nullptr)
  {
    return ezStatus("'MiniDumpWriteDump' function address could not be resolved.");
  }

  ezUInt32 dumpType = MiniDumpWithHandleData | MiniDumpWithModuleHeaders | MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData |
                      MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo;

  if ((opt_FullCrashDumps.GetOptionValue(ezCommandLineOption::LogMode::Always) && dumpTypeOverride == ezDumpType::Auto) || dumpTypeOverride == ezDumpType::MiniDumpWithFullMemory)
  {
    dumpType |= MiniDumpWithFullMemory;
  }

  // make sure the target folder exists
  {
    ezStringBuilder folder = sDumpFile;
    folder.PathParentDirectory();
    if (ezOSFile::CreateDirectoryStructure(folder).Failed())
      return ezStatus("Failed to create output directory structure.");
  }

  HANDLE hFile = CreateFileW(ezDosDevicePath(sDumpFile), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (hFile == INVALID_HANDLE_VALUE)
  {
    return ezStatus(ezFmt("Creating dump file '{}' failed (Error: '{}').", sDumpFile, ezArgErrorCode(GetLastError())));
  }

  EZ_SCOPE_EXIT(CloseHandle(hFile););

  MINIDUMP_EXCEPTION_INFORMATION exceptionParam;
  exceptionParam.ThreadId = GetCurrentThreadId(); // only valid for WriteOwnProcessMiniDump()
  exceptionParam.ExceptionPointers = pExceptionInfo;
  exceptionParam.ClientPointers = TRUE;

  if (MiniDumpWriteDumpFunc(
        hProcess, uiProcessID, hFile, (MINIDUMP_TYPE)dumpType, pExceptionInfo != nullptr ? &exceptionParam : nullptr, nullptr, nullptr) == FALSE)
  {
    return ezStatus(ezFmt("Writing dump file failed: '{}'.", ezArgErrorCode(GetLastError())));
  }

  return EZ_SUCCESS;
}

ezStatus ezMiniDumpUtils::WriteOwnProcessMiniDump(ezStringView sDumpFile, struct _EXCEPTION_POINTERS* pExceptionInfo, ezDumpType dumpTypeOverride)
{
  return WriteProcessMiniDump(sDumpFile, GetCurrentProcessId(), GetCurrentProcess(), pExceptionInfo, dumpTypeOverride);
}

ezStatus ezMiniDumpUtils::WriteExternalProcessMiniDump(ezStringView sDumpFile, ezUInt32 uiProcessID, ezMinWindows::HANDLE hProcess, ezDumpType dumpTypeOverride)
{
  return WriteProcessMiniDump(sDumpFile, uiProcessID, hProcess, nullptr, dumpTypeOverride);
}

ezStatus ezMiniDumpUtils::WriteExternalProcessMiniDump(ezStringView sDumpFile, ezUInt32 uiProcessID, ezDumpType dumpTypeOverride)
{
  HANDLE hProcess = ezMiniDumpUtils::GetProcessHandleWithNecessaryRights(uiProcessID);

  if (hProcess == nullptr)
  {
    return ezStatus("Cannot access process for mini-dump writing (PID invalid or not enough rights).");
  }

  return WriteProcessMiniDump(sDumpFile, uiProcessID, hProcess, nullptr, dumpTypeOverride);
}

ezStatus ezMiniDumpUtils::LaunchMiniDumpTool(ezStringView sDumpFile, ezDumpType dumpTypeOverride)
{
  ezStringBuilder sDumpToolPath = ezOSFile::GetApplicationDirectory();
  sDumpToolPath.AppendPath("ezMiniDumpTool.exe");
  sDumpToolPath.MakeCleanPath();

  if (!ezOSFile::ExistsFile(sDumpToolPath))
    return ezStatus(ezFmt("ezMiniDumpTool.exe not found in '{}'", sDumpToolPath));

  ezProcessOptions procOpt;
  procOpt.m_sProcess = sDumpToolPath;
  procOpt.m_Arguments.PushBack("-PID");
  procOpt.AddArgument("{}", ezProcess::GetCurrentProcessID());
  procOpt.m_Arguments.PushBack("-f");
  procOpt.m_Arguments.PushBack(sDumpFile);

  if ((opt_FullCrashDumps.GetOptionValue(ezCommandLineOption::LogMode::Always) && dumpTypeOverride == ezDumpType::Auto) || dumpTypeOverride == ezDumpType::MiniDumpWithFullMemory)
  {
    // forward the '-fullcrashdumps' command line argument
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
