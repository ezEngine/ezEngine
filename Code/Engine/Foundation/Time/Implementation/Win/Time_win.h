
static double g_fInvQpcFrequency;

void ezTime::Initialize()
{
  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);

  g_fInvQpcFrequency = 1.0 / double(frequency.QuadPart);
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  static ezAtomicInteger64 s_LastTime;
#endif

ezTime ezTime::Now()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezInt64 LastTime = s_LastTime;
#endif

  LARGE_INTEGER temp;
  QueryPerformanceCounter(&temp);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  EZ_ASSERT_DEV(LastTime <= temp.QuadPart, "Serious problem, Steve. This is like \"Houston, forget that other thing\".\n\n\n"
    "When this happens the PC timer is unreliable. It was probably called from different threads and the clocks on different CPU cores seem to return different results.\n"
    "Under these conditions the engine cannot run reliably, it might crash or act weird.");
  while (!s_LastTime.TestAndSet(LastTime, ezMath::Max(LastTime, temp.QuadPart)))
  {
    LastTime = s_LastTime;
  }
#endif

  return ezTime::Seconds(double(temp.QuadPart) * g_fInvQpcFrequency);
}

