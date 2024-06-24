#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

#  include <Foundation/System/StackTracer.h>

void ezStackTracer::OnPluginEvent(const ezPluginEvent& e)
{
  EZ_IGNORE_UNUSED(e);
}

// static
ezUInt32 ezStackTracer::GetStackTrace(ezArrayPtr<void*>& trace, void* pContext)
{
  EZ_IGNORE_UNUSED(trace);
  EZ_IGNORE_UNUSED(pContext);

  return 0;
}

// static
void ezStackTracer::ResolveStackTrace(const ezArrayPtr<void*>& trace, PrintFunc printFunc)
{
  EZ_IGNORE_UNUSED(trace);

  char szBuffer[512] = "Stack Traces are currently not supported on UWP";

  printFunc(szBuffer);
}

#endif
