
#ifdef EZ_STACKTRACER_POSIX_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_STACKTRACER_POSIX_INL_H_INCLUDED


#include <execinfo.h>

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
  
  if (ppSymbols != NULL)
  {
    for (ezUInt32 i = 0; i < trace.GetCount(); i++)
    {
      strlcpy(szBuffer, ppSymbols[i], EZ_ARRAY_SIZE(szBuffer));
      strlcat(szBuffer, "\n", EZ_ARRAY_SIZE(szBuffer));
      
      (*printFunc)(szBuffer);
    }
  
    free(ppSymbols);
  }
  
}

