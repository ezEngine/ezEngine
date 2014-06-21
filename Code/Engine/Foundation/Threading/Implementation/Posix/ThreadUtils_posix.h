
#ifdef EZ_THREADUTILS_POSIX_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_THREADUTILS_POSIX_INL_H_INCLUDED


// Posix implementation of thread helper functions

#include <pthread.h>

static pthread_t g_MainThread = (pthread_t)0;

void ezThreadUtils::Initialize()
{
  g_MainThread = pthread_self();
}

void ezThreadUtils::YieldTimeSlice()
{
  sched_yield();
}

void ezThreadUtils::Sleep(ezUInt32 uiMilliSeconds)
{
  timespec SleepTime;
  SleepTime.tv_sec = uiMilliSeconds / 1000;
  SleepTime.tv_nsec = (uiMilliSeconds * 1000000LL) % 1000000000LL;
  nanosleep(&SleepTime, nullptr);
}

//ezThreadHandle ezThreadUtils::GetCurrentThreadHandle()
//{
//  return pthread_self();
//}

ezThreadID ezThreadUtils::GetCurrentThreadID()
{
  return pthread_self();
}

bool ezThreadUtils::IsMainThread()
{
  return pthread_self() == g_MainThread;
}

