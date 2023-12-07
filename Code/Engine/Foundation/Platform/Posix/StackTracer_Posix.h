#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/StackTracer.h>

#include <Foundation/Math/Math.h>

#if __has_include(<execinfo.h>)
#  include <execinfo.h>
#  define HAS_EXECINFO 1
#endif

void ezStackTracer::OnPluginEvent(const ezPluginEvent& e)
{
}

// static
ezUInt32 ezStackTracer::GetStackTrace(ezArrayPtr<void*>& trace, void* pContext)
{
#if HAS_EXECINFO
  return backtrace(trace.GetPtr(), trace.GetCount());
#else
  return 0;
#endif
}

// static
void ezStackTracer::ResolveStackTrace(const ezArrayPtr<void*>& trace, PrintFunc printFunc)
{
#if HAS_EXECINFO
  char szBuffer[512];

  char** ppSymbols = backtrace_symbols(trace.GetPtr(), trace.GetCount());

  if (ppSymbols != nullptr)
  {
    for (ezUInt32 i = 0; i < trace.GetCount(); i++)
    {
      size_t uiLen = ezMath::Min(strlen(ppSymbols[i]), static_cast<size_t>(EZ_ARRAY_SIZE(szBuffer)) - 2);
      memcpy(szBuffer, ppSymbols[i], uiLen);
      szBuffer[uiLen] = '\n';
      szBuffer[uiLen + 1] = '\0';

      printFunc(szBuffer);
    }

    free(ppSymbols);
  }
#else
  printFunc("Could not record stack trace on this Linux system, because execinfo.h is not available.");
#endif
}
