
#include <unistd.h>

void ezSystemInformation::Initialize()
{
  // Get system information via various APIs
  s_SystemInformation.m_uiCPUCoreCount = sysconf(_SC_NPROCESSORS_ONLN);

  ezUInt64 uiPageCount = sysconf(_SC_PHYS_PAGES);
  ezUInt64 uiPageSize = sysconf(_SC_PAGE_SIZE);

  s_SystemInformation.m_uiInstalledMainMemory = uiPageCount * uiPageSize;
  s_SystemInformation.m_uiMemoryPageSize = uiPageSize;

  // Not correct for 32 bit process on 64 bit system
#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  s_SystemInformation.m_b64BitOS = true;
#else
  s_SystemInformation.m_b64BitOS = false;
  #error "32 Bit builds are not supported on OSX"
#endif

#if defined BUILDSYSTEM_CONFIGURATION
  s_SystemInformation.m_szBuildConfiguration = BUILDSYSTEM_CONFIGURATION;
#else
  s_SystemInformation.m_szBuildConfiguration = "undefined";
#endif

  // Each posix system should have its correct name so they can be distinguished.
#if EZ_ENABLED(EZ_PLATFORM_LINUX)
  s_SystemInformation.m_szPlatformName = "Linux";
#else
  #error "Platform name not defined on current posix platform"
#endif

  //  Get host name
  if (gethostname(s_SystemInformation.m_sHostName, sizeof(s_SystemInformation.m_sHostName)) == -1)
  {
    strcpy(s_SystemInformation.m_sHostName, "");
  }
}

