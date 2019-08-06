#ifdef EZ_STACKTRACER_ANDROID_INL_H_INCLUDED
#error "This file must not be included twice."
#endif

#define EZ_STACKTRACER_ANDROID_INL_H_INCLUDED

#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

// TODO: Use unwind.h for a stack trace implementation
// https://stackoverflow.com/questions/8115192/android-ndk-getting-the-backtrace

void ezStackTracer::OnPluginEvent(const ezPlugin::PluginEvent& e) {}

// static
ezUInt32 ezStackTracer::GetStackTrace(ezArrayPtr<void*>& trace, void* pContext)
{
  return 0;
}

// static
void ezStackTracer::ResolveStackTrace(const ezArrayPtr<void*>& trace, PrintFunc printFunc)
{
}

