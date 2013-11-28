
#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

static mach_timebase_info_data_t g_TimebaseInfo;

void ezSystemTime::Initialize()
{
  mach_timebase_info(&g_TimebaseInfo);
}

void ezSystemTime::Shutdown()
{
}

ezTime ezSystemTime::Now()
{
  // mach_absolute_time() returns nanoseconds after factoring in the mach_timebase_info_data_t
  return ezTime::Seconds( (double)((mach_absolute_time() * g_TimebaseInfo.numer) / g_TimebaseInfo.denom) / 1000000000LL);
}
