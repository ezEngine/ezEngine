#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/StackTracer.h>

static LONG WINAPI ezCrashHandlerFunc(struct _EXCEPTION_POINTERS* pExceptionInfo)
{
  if (ezCrashHandler::GetCrashHandler() != nullptr)
  {
    ezCrashHandler::GetCrashHandler()->HandleCrash(pExceptionInfo);
  }

  return EXCEPTION_CONTINUE_SEARCH;
}

void ezCrashHandler::SetCrashHandler(ezCrashHandler* pHandler)
{
  s_pActiveHandler = pHandler;

  if (s_pActiveHandler != nullptr)
  {
    SetUnhandledExceptionFilter(ezCrashHandlerFunc);
  }
  else
  {
    SetUnhandledExceptionFilter(nullptr);
  }
}

void ezCrashHandler_WriteMiniDump::WriteOwnProcessMiniDump(void* pOsSpecificData)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  ezMiniDumpUtils::WriteOwnProcessMiniDump(m_sDumpFilePath, (_EXCEPTION_POINTERS*)pOsSpecificData);
#endif
}

void ezCrashHandler_WriteMiniDump::PrintStackTrace(void* pOsSpecificData)
{
  _EXCEPTION_POINTERS* pExceptionInfo = (_EXCEPTION_POINTERS*)pOsSpecificData;

  ezLog::Printf("***Unhandled Exception:***\n");
  ezLog::Printf("Exception: %08x", (ezUInt32)pExceptionInfo->ExceptionRecord->ExceptionCode);

  {
    ezLog::Printf("\n\n***Stack Trace:***\n");
    void* pBuffer[64];
    ezArrayPtr<void*> tempTrace(pBuffer);
    const ezUInt32 uiNumTraces = ezStackTracer::GetStackTrace(tempTrace, pExceptionInfo->ContextRecord);

    ezStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &PrintHelper);
  }
}
