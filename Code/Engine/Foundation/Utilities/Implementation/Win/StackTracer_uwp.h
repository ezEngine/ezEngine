
#ifdef EZ_STACKTRACER_UWP_INL_H_INCLUDED
  #error "This file must not be included twice."
#endif

#define EZ_STACKTRACER_UWP_INL_H_INCLUDED

//static
ezUInt32 ezStackTracer::GetStackTrace(ezArrayPtr<void*>& trace, void* pContext)
{
  return 0;
}

//static
void ezStackTracer::ResolveStackTrace(const ezArrayPtr<void*>& trace, PrintFunc printFunc)
{
  char szBuffer[512] = "Stack Traces are currently not supported on UWP";

  (*printFunc)(szBuffer);
}

