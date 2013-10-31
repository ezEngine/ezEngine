
static LARGE_INTEGER g_qpcFrequency = {0};

void ezSystemTime::Initialize()
{
  QueryPerformanceFrequency(&g_qpcFrequency);
}

void ezSystemTime::Shutdown()
{
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  static LARGE_INTEGER s_LastTime = { 0 };
#endif

ezTime ezSystemTime::Now()
{
  LARGE_INTEGER temp;

  QueryPerformanceCounter(&temp);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  EZ_ASSERT(s_LastTime.QuadPart <= temp.QuadPart, "Serious problem, Steve. This is like \"Houston, forget that other thing\".\n\n\n"
    "When this happens the PC timer is unreliable. It was probably called from different threads and the clocks on different CPU cores seem to return different results.\n"
    "Under these conditions the engine cannot run reliably, it might crash or act weird.");
  s_LastTime = temp;
#endif

  return ezTime::Seconds((double(temp.QuadPart) / double(g_qpcFrequency.QuadPart)));
}
