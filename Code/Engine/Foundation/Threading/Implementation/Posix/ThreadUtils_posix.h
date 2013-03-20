
#ifdef EZ_THREADUTILS_POSIX_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_THREADUTILS_POSIX_INL_H_INCLUDED

// Windows implementation of thread helper functions

static pthread_t g_MainThread = (pthread_t)0;

void ezThreadUtils::Initialize()
{
  g_MainThread = pthread_self();
}

void ezThreadUtils::Shutdown()
{
}

void ezThreadUtils::YieldTimeSlice()
{
  Sleep(0);
}

void ezThreadUtils::Sleep(ezUInt32 uiMilliSeconds)
{
  timespec SleepTime;
  SleepTime.tv_sec = 0;
  SleepTime.tv_nsec = uiMilliSeconds * 1000;
  nanosleep(&SleepTime, NULL);
}

bool ezThreadUtils::IsMainThread()
{
  return pthread_self() == g_MainThread;
}

ezThreadHandle ezThreadUtils::GetCurrentThreadHandle()
{
  return pthread_self();
}