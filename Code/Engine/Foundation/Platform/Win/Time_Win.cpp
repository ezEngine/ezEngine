#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <Foundation/Time/Time.h>

static double g_fInvQpcFrequency;

void ezTime::Initialize()
{
  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);

  g_fInvQpcFrequency = 1.0 / double(frequency.QuadPart);
}

#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
thread_local ezInt64 s_LastTime;
#  endif

ezTime ezTime::Now()
{
  LARGE_INTEGER temp;
  QueryPerformanceCounter(&temp);

#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  EZ_ASSERT_DEBUG(s_LastTime <= temp.QuadPart, "Serious problem, Steve. This is like \"Houston, forget that other thing\".\n\n\n"
                                               "When this happens the PC timer is unreliable. It was probably called from different threads "
                                               "and the clocks on different CPU cores seem to return different results.\n"
                                               "Under these conditions the engine cannot run reliably, it might crash or act weird.");
  s_LastTime = temp.QuadPart;
#  endif

  return ezTime::MakeFromSeconds(double(temp.QuadPart) * g_fInvQpcFrequency);
}

#endif
