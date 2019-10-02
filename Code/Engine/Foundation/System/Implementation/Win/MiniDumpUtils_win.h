#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Win/MinWindows.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/System/ProcessGroup.h>
#include <Foundation/Types/ScopeExit.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#include <Dbghelp.h>
#include <Shlwapi.h>
#include <tchar.h>
#include <werapi.h>

typedef BOOL(WINAPI* MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
  PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
  PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

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

ezStatus ezMiniDumpUtils::WriteProcessMiniDump(
  const char* szDumpFile, ezUInt32 uiProcessID, ezMinWindows::HANDLE hProcess, struct _EXCEPTION_POINTERS* pExceptionInfo)
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

  const ezUInt32 dumpType(/*MiniDumpWithFullMemory |*/ MiniDumpWithHandleData | MiniDumpWithModuleHeaders | MiniDumpWithUnloadedModules |
                          MiniDumpWithProcessThreadData | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo);

  // make sure the target folder exists
  {
    ezStringBuilder folder = szDumpFile;
    folder.PathParentDirectory();
    ezOSFile::CreateDirectoryStructure(folder);
  }

  HANDLE hFile = CreateFileW(ezStringWChar(szDumpFile).GetData(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (hFile == INVALID_HANDLE_VALUE)
  {
    return ezStatus(ezFmt("Creating dump file '{}' failed (Error: '{}').", szDumpFile, ezArgErrorCode(GetLastError())));
  }

  EZ_SCOPE_EXIT(CloseHandle(hFile););

  MINIDUMP_EXCEPTION_INFORMATION exceptionParam;
  exceptionParam.ThreadId = GetCurrentThreadId(); // only valid for WriteOwnProcessMiniDump()
  exceptionParam.ExceptionPointers = pExceptionInfo;
  exceptionParam.ClientPointers = TRUE;

  if (MiniDumpWriteDumpFunc(hProcess, uiProcessID, hFile, (MINIDUMP_TYPE)dumpType, pExceptionInfo != nullptr ? &exceptionParam : nullptr,
        nullptr, nullptr) == FALSE)
  {
    return ezStatus(ezFmt("Writing dump file failed: '{}'.", ezArgErrorCode(GetLastError())));
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezMiniDumpUtils::WriteOwnProcessMiniDump(const char* szDumpFile, struct _EXCEPTION_POINTERS* pExceptionInfo)
{
  return WriteProcessMiniDump(szDumpFile, GetCurrentProcessId(), GetCurrentProcess(), pExceptionInfo);
}

ezStatus ezMiniDumpUtils::WriteExternalProcessMiniDump(const char* szDumpFile, ezUInt32 uiProcessID, ezMinWindows::HANDLE hProcess)
{
  return WriteProcessMiniDump(szDumpFile, uiProcessID, hProcess, nullptr);
}

#endif

bool ezMiniDumpUtils::IsSupported()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  return true;
#else
  return false;
#endif
}

ezStatus ezMiniDumpUtils::WriteExternalProcessMiniDump(const char* szDumpFile, ezUInt32 uiProcessID)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  HANDLE hProcess = ezMiniDumpUtils::GetProcessHandleWithNecessaryRights(uiProcessID);

  if (hProcess == nullptr)
  {
    return ezStatus("Cannot access process for mini-dump writing (PID invalid or not enough rights).");
  }

  return WriteProcessMiniDump(szDumpFile, uiProcessID, hProcess, nullptr);

#else
  return ezStatus("Not implemented on UPW");
#endif
}

ezStatus ezMiniDumpUtils::LaunchMiniDumpTool(const char* szDumpFile)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  ezStringBuilder sDumpToolPath = ezOSFile::GetApplicationDirectory();
  sDumpToolPath.AppendPath("MiniDumpTool.exe");
  sDumpToolPath.MakeCleanPath();

  if (!ezOSFile::ExistsFile(sDumpToolPath))
    return ezStatus(ezFmt("MiniDumpTool.exe not found in '{}'", sDumpToolPath));

  ezProcessOptions procOpt;
  procOpt.m_sProcess = sDumpToolPath;
  procOpt.m_Arguments.PushBack("-PID");
  procOpt.AddArgument("{}", ezProcess::GetCurrentProcessID());
  procOpt.m_Arguments.PushBack("-f");
  procOpt.m_Arguments.PushBack(szDumpFile);

  ezProcessGroup proc;
  if (proc.Launch(procOpt).Failed())
    return ezStatus(ezFmt("Failed to launch '{}'", sDumpToolPath));

  if (proc.WaitToFinish().Failed())
    return ezStatus("Waiting for MiniDumpTool to finish failed.");

  return ezStatus(EZ_SUCCESS);

#else
  return ezStatus("Not implemented on UPW");
#endif
}
