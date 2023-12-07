#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Foundation/Threading/ThreadUtils.h>

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

#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Platform_Win_ThreadUtils_Win);

