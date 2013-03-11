
static LARGE_INTEGER g_qpcFrequency = {0};

void ezSystemTime::Initialize()
{
  QueryPerformanceFrequency(&g_qpcFrequency);
}

void ezSystemTime::Shutdown()
{
}

ezTime ezSystemTime::Now()
{
  LARGE_INTEGER temp;

  QueryPerformanceCounter(&temp);

  return ezTime((double(temp.QuadPart) / double(g_qpcFrequency.QuadPart)));
}
