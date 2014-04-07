
#ifdef EZ_STACKTRACER_POSIX_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_STACKTRACER_POSIX_INL_H_INCLUDED


#include <execinfo.h>
#include <Foundation/Math/Math.h>

//static
ezUInt32 ezStackTracer::GetStackTrace(ezArrayPtr<void*>& trace)
{
  int iSymbols = backtrace(trace.GetPtr(), trace.GetCount());
  
  return iSymbols;
}

//static
void ezStackTracer::ResolveStackTrace(const ezArrayPtr<void*>& trace, PrintFunc printFunc)
{
  char szBuffer[512];
  
  char** ppSymbols = backtrace_symbols(trace.GetPtr(), trace.GetCount());
  
  if (ppSymbols != nullptr)
  {
    for (ezUInt32 i = 0; i < trace.GetCount(); i++)
    {
      int iLen = ezMath::Min(strlen(ppSymbols[i]), (size_t)EZ_ARRAY_SIZE(szBuffer) - 2);
      memcpy(szBuffer, ppSymbols[i], iLen);
      szBuffer[iLen] = '\n';
      szBuffer[iLen + 1] = '\0';
      
      (*printFunc)(szBuffer);
    }
  
    free(ppSymbols);
  }
  
}

