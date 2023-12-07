#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_OSX)

#  include <Foundation/System/SystemInformation.h>

#  include <assert.h>
#  include <stdbool.h>
#  include <sys/sysctl.h>
#  include <sys/types.h>
#  include <sys/vmmeter.h>
#  include <unistd.h>

// https://developer.apple.com/library/archive/qa/qa1361/_index.html
// Returns true if the current process is being debugged (either
// running under the debugger or has a debugger attached post facto).
bool ezSystemInformation::IsDebuggerAttached()
{
  int junk;
  int mib[4];
  struct kinfo_proc info;
  size_t size;

  // Initialize the flags so that, if sysctl fails for some bizarre
  // reason, we get a predictable result.
  info.kp_proc.p_flag = 0;

  // Initialize mib, which tells sysctl the info we want, in this case
  // we're looking for information about a specific process ID.
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PID;
  mib[3] = getpid();

  // Call sysctl.
  size = sizeof(info);
  junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
  assert(junk == 0);

  // We're being debugged if the P_TRACED flag is set.
  return ((info.kp_proc.p_flag & P_TRACED) != 0);
}

void ezSystemInformation::Initialize()
{
  if (s_SystemInformation.m_bIsInitialized)
    return;

  s_SystemInformation.m_CpuFeatures.Detect();

  // Get system information via various APIs
  s_SystemInformation.m_uiCPUCoreCount = static_cast<ezUInt32>(sysconf(_SC_NPROCESSORS_ONLN));

  ezUInt64 uiPageSize = sysconf(_SC_PAGE_SIZE);

  s_SystemInformation.m_uiMemoryPageSize = static_cast<ezUInt32>(uiPageSize);

  int mib[2];
  int64_t iPhysicalMemory = 0;
  size_t uiLength = sizeof(iPhysicalMemory);

  mib[0] = CTL_HW;
  mib[1] = HW_MEMSIZE;
  sysctl(mib, 2, &iPhysicalMemory, &uiLength, nullptr, 0);

  s_SystemInformation.m_uiInstalledMainMemory = iPhysicalMemory;

  // Not correct for 32 bit process on 64 bit system
#  if EZ_ENABLED(EZ_PLATFORM_64BIT)
  s_SystemInformation.m_bB64BitOS = true;
#  else
  s_SystemInformation.m_bB64BitOS = false;
#    error "32 Bit builds are not supported on OSX"
#  endif

  s_SystemInformation.m_szPlatformName = "OSX";
#  if defined BUILDSYSTEM_BUILDTYPE
  s_SystemInformation.m_szBuildConfiguration = BUILDSYSTEM_BUILDTYPE;
#  else
  s_SystemInformation.m_szBuildConfiguration = "undefined";
#  endif

  //  Get host name
  if (gethostname(s_SystemInformation.m_sHostName, sizeof(s_SystemInformation.m_sHostName)) == -1)
  {
    strcpy(s_SystemInformation.m_sHostName, "");
  }

  s_SystemInformation.m_bIsInitialized = true;
}

ezUInt64 ezSystemInformation::GetAvailableMainMemory() const
{
  struct vmtotal vmt = {0};
  size_t vmt_size = sizeof(vmt);

  int rc = sysctlbyname("vm.vmtotal", &vmt, &vmt_size, NULL, 0);
  if (rc < 0)
  {
    perror("sysctlbyname");
    return 0;
  }

  return static_cast<ezUInt64>(s_SystemInformation.m_uiMemoryPageSize) * static_cast<ezUInt64>(vmt.t_free);
}

float ezSystemInformation::GetCPUUtilization() const
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return 0.0f;
}

#endif
