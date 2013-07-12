
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
void ezStackTracer::DumpStackTrace(const ezArrayPtr<void*>& trace)
{
  char** ppSymbols = backtrace_symbols(trace.GetPtr(), trace.GetCount());
  
  if(ppSymbols != NULL)
  {
    for(ezUInt32 i = 0; i < trace.GetCount(); i++)
    {
      printf("%s\n", ppSymbols[i]);
    }
  
    free(ppSymbols);
  }
  
}
