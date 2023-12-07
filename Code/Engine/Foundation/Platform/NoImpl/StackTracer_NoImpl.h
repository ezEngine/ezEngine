#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

void ezStackTracer::OnPluginEvent(const ezPluginEvent& e) {}

ezUInt32 ezStackTracer::GetStackTrace(ezArrayPtr<void*>& trace, void* pContext)
{
  return 0;
}

void ezStackTracer::ResolveStackTrace(const ezArrayPtr<void*>& trace, PrintFunc printFunc)
{
}
