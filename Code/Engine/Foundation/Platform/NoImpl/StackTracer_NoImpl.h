#include <Foundation/System/StackTracer.h>

void ezStackTracer::OnPluginEvent(const ezPluginEvent& e)
{
}

ezUInt32 ezStackTracer::GetStackTrace(ezArrayPtr<void*>& trace, void* pContext)
{
  return 0;
}

void ezStackTracer::ResolveStackTrace(const ezArrayPtr<void*>& trace, PrintFunc printFunc)
{
}
