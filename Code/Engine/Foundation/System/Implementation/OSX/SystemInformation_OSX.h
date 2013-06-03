
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>

void ezSystemInformation::Initialize()
{
  // Get system information via various APIs
  s_SystemInformation.m_uiCPUCoreCount = sysconf(_SC_NPROCESSORS_ONLN);

  ezUInt64 uiPageSize = sysconf(_SC_PAGE_SIZE);

  s_SystemInformation.m_uiMemoryPageSize = uiPageSize;

  int mib[2];
  int64_t iPhysicalMemory = 0;
  size_t uiLength = sizeof(iPhysicalMemory);
  
  mib[0] = CTL_HW; mib[1] = HW_MEMSIZE;
  sysctl(mib, 2, &iPhysicalMemory, &uiLength, NULL, 0);
  
  s_SystemInformation.m_uiInstalledMainMemory = iPhysicalMemory;
  
  
  
  
  // TODO: Not correct for 32 bit process on 64 bit system
#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  s_SystemInformation.m_b64BitOS = true;
#else
  s_SystemInformation.m_b64BitOS = false;
#endif
}
