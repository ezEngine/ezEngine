#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

// Windows implementation of thread helper functions

static DWORD g_uiMainThreadID = 0xFFFFFFFF;

void ezThreadUtils::Initialize()
{
  g_uiMainThreadID = GetCurrentThreadId();
}

void ezThreadUtils::YieldTimeSlice()
{
  ::Sleep(0);
}

void ezThreadUtils::YieldHardwareThread()
{
  YieldProcessor();
}

void ezThreadUtils::Sleep(const ezTime& duration)
{
  ::Sleep((DWORD)duration.GetMilliseconds());
}

ezThreadID ezThreadUtils::GetCurrentThreadID()
{
  return ::GetCurrentThreadId();
}

bool ezThreadUtils::IsMainThread()
{
  return GetCurrentThreadID() == g_uiMainThreadID;
}
