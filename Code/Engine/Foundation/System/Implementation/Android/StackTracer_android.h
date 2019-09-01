#ifdef EZ_STACKTRACER_ANDROID_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define EZ_STACKTRACER_ANDROID_INL_H_INCLUDED

#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <unwind.h>
#include <dlfcn.h>

void ezStackTracer::OnPluginEvent(const ezPlugin::PluginEvent& e) {}

struct Backtrace
{
  ezUInt32 uiPos = 0;
  ezArrayPtr<void*> trace;
};

static _Unwind_Reason_Code BacktraceCallback(struct _Unwind_Context* pContext, void* pData)
{
  Backtrace& backtrace = *(Backtrace*)pData;

  if (backtrace.uiPos < backtrace.trace.GetCount())
  {
    backtrace.trace[backtrace.uiPos] = reinterpret_cast<void*>(_Unwind_GetIP(pContext));
    backtrace.uiPos++;
    return _URC_NO_REASON;
  }
  else
  {
    return _URC_END_OF_STACK;
  }
}
// static
ezUInt32 ezStackTracer::GetStackTrace(ezArrayPtr<void*>& trace, void* pContext)
{
  Backtrace backtrace;
  backtrace.trace = trace;
  _Unwind_Reason_Code res = _Unwind_Backtrace(BacktraceCallback, &backtrace);
  return backtrace.uiPos;
}

// static
void ezStackTracer::ResolveStackTrace(const ezArrayPtr<void*>& trace, PrintFunc printFunc)
{
  char szBuffer[512];
  for (ezUInt32 i = 0; i < trace.GetCount(); i++)
  {
    Dl_info info;
    if (dladdr(trace[i], &info) && info.dli_sname)
    {
      int iLen = ezMath::Min(strlen(info.dli_sname), (size_t)EZ_ARRAY_SIZE(szBuffer) - 2);
      memcpy(szBuffer, info.dli_sname, iLen);
      szBuffer[iLen] = '\n';
      szBuffer[iLen + 1] = '\0';
      (*printFunc)(szBuffer);
    }
    else
    {
      (*printFunc)("Unresolved stack.\n");
    }
  }
}
