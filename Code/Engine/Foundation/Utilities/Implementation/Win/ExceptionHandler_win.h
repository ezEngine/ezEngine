#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Dbghelp.h>
#include <Shlwapi.h>
#include <tchar.h>
#include <werapi.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
typedef BOOL(WINAPI* MINIDUMPWRITEDUMP)(_In_ HANDLE hProcess, _In_ DWORD ProcessId, _In_ HANDLE hFile, _In_ MINIDUMP_TYPE DumpType,
  _In_opt_ PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, _In_opt_ PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
  _In_opt_ PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
#endif

LONG WINAPI ezExceptionHandler::DefaultExceptionHandler(struct _EXCEPTION_POINTERS* pExceptionInfo)
{
  Print("***Unhandled Exception:***\n");
  Print("Exception: %08x", (ezUInt32)pExceptionInfo->ExceptionRecord->ExceptionCode);

  {
    Print("\n\n***Stack Trace:***\n");
    void* pBuffer[64];
    ezArrayPtr<void*> tempTrace(pBuffer);
    const ezUInt32 uiNumTraces = ezStackTracer::GetStackTrace(tempTrace, pExceptionInfo->ContextRecord);

    ezStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &PrintHelper);
  }

  WriteDump(pExceptionInfo);
  return EXCEPTION_CONTINUE_SEARCH;
}

void ezExceptionHandler::SetExceptionHandler(EZ_TOP_LEVEL_EXCEPTION_HANDLER handler, const char* appName, const char* absDumpPath)
{
  s_appName = appName;
  s_absDumpPath = absDumpPath;
  SetUnhandledExceptionFilter(handler);
}

ezResult ezExceptionHandler::WriteDump(struct _EXCEPTION_POINTERS* exceptionInfo)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  ezStringBuilder path;
  const ezDateTime dt = ezTimestamp::CurrentTimestamp();
  path.Format("{0}/{1}_", s_absDumpPath, s_appName);
  path.AppendFormat("{0}-{1}-{2}_{3}-{4}-{5}-{6}.dmp", dt.GetYear(), ezArgU(dt.GetMonth(), 2, true), ezArgU(dt.GetDay(), 2, true),
    ezArgU(dt.GetHour(), 2, true), ezArgU(dt.GetMinute(), 2, true), ezArgU(dt.GetSecond(), 2, true),
    ezArgU(dt.GetMicroseconds() / 1000, 3, true));


  HMODULE hDLL = ::LoadLibraryA("dbghelp.dll");
  if (!hDLL)
  {
    Print("Failed to load 'dbghelp.dll'. Can't write dumps.");
    return EZ_FAILURE;
  }
  MINIDUMPWRITEDUMP MiniDumpWriteDumpFunc = (MINIDUMPWRITEDUMP)::GetProcAddress(hDLL, "MiniDumpWriteDump");
  if (!MiniDumpWriteDumpFunc)
  {
    Print("Failed to resolve address of 'MiniDumpWriteDump'. Can't write dumps.");
    return EZ_FAILURE;
  }

  int dumpType(/*MiniDumpWithFullMemory |*/ MiniDumpWithHandleData | MiniDumpWithModuleHeaders | MiniDumpWithUnloadedModules |
               MiniDumpWithProcessThreadData | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo);

  HANDLE hFile = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile != INVALID_HANDLE_VALUE)
  {
    EZ_SCOPE_EXIT(CloseHandle(hFile););
    MINIDUMP_EXCEPTION_INFORMATION exceptionParam;
    exceptionParam.ThreadId = GetCurrentThreadId();
    exceptionParam.ExceptionPointers = exceptionInfo;
    exceptionParam.ClientPointers = TRUE;

    if (MiniDumpWriteDumpFunc(GetCurrentProcess(), GetCurrentProcessId(), hFile, (MINIDUMP_TYPE)dumpType,
          exceptionInfo != nullptr ? &exceptionParam : nullptr, nullptr, nullptr) == TRUE)
    {
      Print("Dump file written to '%s'.", path.GetData());
      return EZ_SUCCESS;
    }
    else
    {
      Print("Writing dump file '%s' failed (Error: '%d').", path.GetData(), GetLastError());
    }
  }
  else
  {
    Print("Creating dump file '%s' failed (Error: '%d').", path.GetData(), GetLastError());
  }
#endif
  return EZ_FAILURE;
}
