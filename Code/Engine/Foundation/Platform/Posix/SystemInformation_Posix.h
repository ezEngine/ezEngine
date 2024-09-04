#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/SystemInformation.h>

#include <Foundation/IO/OSFile.h>
#include <unistd.h>

bool ezSystemInformation::IsDebuggerAttached()
{
#if EZ_ENABLED(EZ_PLATFORM_WEB)
  return false;
#else

  ezOSFile status;
  if (status.Open("/proc/self/status", ezFileOpenMode::Read).Failed())
  {
    return false;
  }


  char buffer[2048];
  ezUInt64 numBytesRead = status.Read(buffer, EZ_ARRAY_SIZE(buffer));
  status.Close();


  ezStringView contents(buffer, numBytesRead);
  const char* tracerPid = contents.FindSubString("TracerPid:");
  if (tracerPid == nullptr)
  {
    return false;
  }

  tracerPid += 10; // Skip TracerPid:

  while (*tracerPid == ' ' || *tracerPid == '\t')
  {
    tracerPid++;
  }

  return *tracerPid == '0' ? false : true;
#endif
}

void ezSystemInformation::Initialize()
{
  if (s_SystemInformation.m_bIsInitialized)
    return;

  s_SystemInformation.m_CpuFeatures.Detect();

  // Get system information via various APIs
  s_SystemInformation.m_uiCPUCoreCount = sysconf(_SC_NPROCESSORS_ONLN);

  ezUInt64 uiPageCount = sysconf(_SC_PHYS_PAGES);
  ezUInt64 uiPageSize = sysconf(_SC_PAGE_SIZE);

  s_SystemInformation.m_uiInstalledMainMemory = uiPageCount * uiPageSize;
  s_SystemInformation.m_uiMemoryPageSize = uiPageSize;

  // Not correct for 32 bit process on 64 bit system
#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  s_SystemInformation.m_bB64BitOS = true;
#else
  s_SystemInformation.m_bB64BitOS = false;
#  if EZ_ENABLED(EZ_PLATFORM_OSX)
#    error "32 Bit builds are not supported on OSX"
#  endif
#endif

#if defined BUILDSYSTEM_BUILDTYPE
  s_SystemInformation.m_szBuildConfiguration = BUILDSYSTEM_BUILDTYPE;
#else
  s_SystemInformation.m_szBuildConfiguration = "undefined";
#endif

  // Each posix system should have its correct name so they can be distinguished.
#if EZ_ENABLED(EZ_PLATFORM_LINUX)
  s_SystemInformation.m_szPlatformName = "Linux";
#elif EZ_ENABLED(EZ_PLATFORM_ANDROID)
  s_SystemInformation.m_szPlatformName = "Android";
#elif EZ_ENABLED(EZ_PLATFORM_WEB)
  s_SystemInformation.m_szPlatformName = "Web";
#else
#  error "Platform name not defined on current posix platform"
#endif

  //  Get host name
  if (gethostname(s_SystemInformation.m_sHostName, sizeof(s_SystemInformation.m_sHostName)) == -1)
  {
    strcpy(s_SystemInformation.m_sHostName, "");
  }

  s_SystemInformation.m_bIsInitialized = true;
}

ezUInt64 ezSystemInformation::GetAvailableMainMemory() const
{
  return static_cast<ezUInt64>(sysconf(_SC_AVPHYS_PAGES)) * static_cast<ezUInt64>(sysconf(_SC_PAGESIZE));
}

float ezSystemInformation::GetCPUUtilization() const
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return 0.0f;
}

#if EZ_ENABLED(EZ_PLATFORM_ARCH_X86)
namespace cpu_x86
{
#  include <cpuid.h>

  void cpuid(int32_t out[4], int32_t eax, int32_t ecx)
  {
    __cpuid_count(eax, ecx, out[0], out[1], out[2], out[3]);
  }

  uint64_t xgetbv(unsigned int index)
  {
    uint32_t eax, edx;
    __asm__ __volatile__("xgetbv"
                         : "=a"(eax), "=d"(edx)
                         : "c"(index));
    return ((uint64_t)edx << 32) | eax;
  }

  bool detect_OS_x64()
  {
    //  We only support x64 on Linux / Mac / etc.
    return true;
  }

} // namespace cpu_x86
#endif
