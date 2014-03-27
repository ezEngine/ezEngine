
#ifdef EZ_THREADUTILS_WIN_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_THREADUTILS_WIN_INL_H_INCLUDED

// Windows implementation of thread helper functions

static DWORD g_uiMainThreadID = 0xFFFFFFFF;

void ezThreadUtils::Initialize()
{
  g_uiMainThreadID = GetCurrentThreadId();
}

void ezThreadUtils::Shutdown()
{
}

void ezThreadUtils::YieldTimeSlice()
{
  ::Sleep(0);
}

void ezThreadUtils::Sleep(ezUInt32 uiMilliSeconds)
{
  ::Sleep(uiMilliSeconds);
}

ezThreadHandle ezThreadUtils::GetCurrentThreadHandle()
{
  return ::GetCurrentThread();
}

bool ezThreadUtils::IsMainThread()
{
  return ::GetCurrentThreadId() == g_uiMainThreadID;
}

