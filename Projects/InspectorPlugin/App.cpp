#include <PCH.h>
#include <Foundation/Threading/Thread.h>

void SetAppStats()
{
  ezStringBuilder sOut;
  const ezSystemInformation info = ezSystemInformation::Get();

  ezStats::SetStat("App/Platform/Name", info.GetPlatformName());

  sOut.Format("%i", info.GetCPUCoreCount());
  ezStats::SetStat("App/CPU Cores", sOut.GetData());

  sOut.Format("%.1f GB", info.GetInstalledMainMemory() / 1024.0f / 1024.0f / 1024.0f);
  ezStats::SetStat("App/RAM", sOut.GetData());

  sOut.Format("%i", ezOSThread::GetThreadCount());
  ezStats::SetStat("App/Threads", sOut.GetData());

  sOut = info.Is64BitOS() ? "64 Bit" : "32 Bit";
  ezStats::SetStat("App/Platform/Architecture", sOut.GetData());

  #if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    sOut = "Debug";
  #elif EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    sOut = "Dev";
  #else
    sOut = "Release";
  #endif
  ezStats::SetStat("App/Platform/Build", sOut.GetData());

  #if EZ_ENABLED(EZ_USE_PROFILING)
    sOut = "Enabled";
  #else
    sOut = "Disabled";
  #endif
  ezStats::SetStat("App/Features/Profiling", sOut.GetData());

  #if EZ_ENABLED(EZ_USE_PROFILING_GPA)
    sOut = "Enabled";
  #else
    sOut = "Disabled";
  #endif
  ezStats::SetStat("App/Features/Intel GPA Support", sOut.GetData());

  #if EZ_ENABLED(EZ_USE_TRACE_ALLOCATOR)
    sOut = "Enabled";
  #else
    sOut = "Disabled";
  #endif
  ezStats::SetStat("App/Features/Trace Allocator", sOut.GetData());

  #if EZ_ENABLED(EZ_SUPPORTS_CPP11)
    sOut = "Enabled";
  #else
    sOut = "Disabled";
  #endif
  ezStats::SetStat("App/Platform/C++ 11 Feature Set", sOut.GetData());

  #if EZ_ENABLED(EZ_PLATFORM_LITTLE_ENDIAN)
    sOut = "Little";
  #else
    sOut = "Big";
  #endif
  ezStats::SetStat("App/Platform/Endianess", sOut.GetData());}


