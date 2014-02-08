#include <time.h>

void ezTime::Initialize()
{
}

ezTime ezTime::Now()
{
  struct timespec sp;
  clock_gettime(CLOCK_MONOTONIC_RAW, &sp);

  return ezTime::Seconds( (double)sp.tv_sec + (double)(sp.tv_nsec / 1000000000.0));
}

