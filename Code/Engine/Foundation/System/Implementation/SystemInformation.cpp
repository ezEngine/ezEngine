#include <Foundation/FoundationPCH.h>

#include <Foundation/System/SystemInformation.h>

// Storage for the current configuration
ezSystemInformation ezSystemInformation::s_SystemInformation;

// Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/System/Implementation/Win/SystemInformation_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
#  include <Foundation/System/Implementation/OSX/SystemInformation_OSX.h>
#elif EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/System/Implementation/Posix/SystemInformation_posix.h>
#else
#  error "System configuration functions are not implemented on current platform"
#endif

/// CPU feature detection code copied from https://github.com/Mysticial/FeatureDetector

#if EZ_ENABLED(EZ_PLATFORM_ARCH_X86)

namespace cpu_x86
{
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#    include <Windows.h>
#    include <intrin.h>

  static void cpuid(int32_t pOut[4], int32_t eax, int32_t ecx)
  {
    __cpuidex(pOut, eax, ecx);
  }
  static __int64 xgetbv(unsigned int x)
  {
    return _xgetbv(x);
  }

  static bool detect_OS_x64()
  {
#    ifdef _M_X64
    return true;
#    else

    BOOL bIsWow64 = FALSE;

    typedef BOOL(WINAPI * LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

    if (NULL != fnIsWow64Process)
    {
      if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
      {
        printf("Error Detecting Operating System.\n");
        printf("Defaulting to 32-bit OS.\n\n");
        bIsWow64 = FALSE;
      }
    }

    return bIsWow64 != 0;
#    endif
  }

#  elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)

#    include <cpuid.h>

  static void cpuid(int32_t out[4], int32_t eax, int32_t ecx)
  {
    __cpuid_count(eax, ecx, out[0], out[1], out[2], out[3]);
  }

  static uint64_t xgetbv(unsigned int index)
  {
    uint32_t eax, edx;
    __asm__ __volatile__("xgetbv"
                         : "=a"(eax), "=d"(edx)
                         : "c"(index));
    return ((uint64_t)edx << 32) | eax;
  }
#    define _XCR_XFEATURE_ENABLED_MASK 0

  static bool detect_OS_x64()
  {
    //  We only support x64 on Linux.
    return true;
  }

#  else
#    error "CPU feature detection functions are not implemented on current platform"
#  endif

  static bool detect_OS_AVX()
  {
    //  Copied from: http://stackoverflow.com/a/22521619/922184

    bool avxSupported = false;

    int cpuInfo[4];
    cpuid(cpuInfo, 1, 0);

    bool osUsesXSAVE_XRSTORE = (cpuInfo[2] & (1 << 27)) != 0;
    bool cpuAVXSuport = (cpuInfo[2] & (1 << 28)) != 0;

    if (osUsesXSAVE_XRSTORE && cpuAVXSuport)
    {
      uint64_t xcrFeatureMask = xgetbv(_XCR_XFEATURE_ENABLED_MASK);
      avxSupported = (xcrFeatureMask & 0x6) == 0x6;
    }

    return avxSupported;
  }

  static bool detect_OS_AVX512()
  {
    if (!detect_OS_AVX())
      return false;

    uint64_t xcrFeatureMask = xgetbv(_XCR_XFEATURE_ENABLED_MASK);
    return (xcrFeatureMask & 0xe6) == 0xe6;
  }

  static std::string get_vendor_string()
  {
    int32_t CPUInfo[4];
    char name[13];

    cpuid(CPUInfo, 0, 0);
    memcpy(name + 0, &CPUInfo[1], 4);
    memcpy(name + 4, &CPUInfo[3], 4);
    memcpy(name + 8, &CPUInfo[2], 4);
    name[12] = '\0';

    return name;
  }

} // namespace cpu_x86

void ezCpuFeatures::Detect()
{
  using namespace cpu_x86;

  //  OS Features
  OS_x64 = detect_OS_x64();
  OS_AVX = detect_OS_AVX();
  OS_AVX512 = detect_OS_AVX512();

  //  Vendor
  std::string vendor(get_vendor_string());
  if (vendor == "GenuineIntel")
  {
    Vendor_Intel = true;
  }
  else if (vendor == "AuthenticAMD")
  {
    Vendor_AMD = true;
  }

  int info[4];
  cpuid(info, 0, 0);
  int nIds = info[0];

  cpuid(info, 0x80000000, 0);
  uint32_t nExIds = info[0];

  //  Detect Features
  if (nIds >= 0x00000001)
  {
    cpuid(info, 0x00000001, 0);
    HW_MMX = (info[3] & ((int)1 << 23)) != 0;
    HW_SSE = (info[3] & ((int)1 << 25)) != 0;
    HW_SSE2 = (info[3] & ((int)1 << 26)) != 0;
    HW_SSE3 = (info[2] & ((int)1 << 0)) != 0;

    HW_SSSE3 = (info[2] & ((int)1 << 9)) != 0;
    HW_SSE41 = (info[2] & ((int)1 << 19)) != 0;
    HW_SSE42 = (info[2] & ((int)1 << 20)) != 0;
    HW_AES = (info[2] & ((int)1 << 25)) != 0;

    HW_AVX = (info[2] & ((int)1 << 28)) != 0;
    HW_FMA3 = (info[2] & ((int)1 << 12)) != 0;

    HW_RDRAND = (info[2] & ((int)1 << 30)) != 0;
  }
  if (nIds >= 0x00000007)
  {
    cpuid(info, 0x00000007, 0);
    HW_AVX2 = (info[1] & ((int)1 << 5)) != 0;

    HW_BMI1 = (info[1] & ((int)1 << 3)) != 0;
    HW_BMI2 = (info[1] & ((int)1 << 8)) != 0;
    HW_ADX = (info[1] & ((int)1 << 19)) != 0;
    HW_MPX = (info[1] & ((int)1 << 14)) != 0;
    HW_SHA = (info[1] & ((int)1 << 29)) != 0;
    HW_RDSEED = (info[1] & ((int)1 << 18)) != 0;
    HW_PREFETCHWT1 = (info[2] & ((int)1 << 0)) != 0;
    HW_RDPID = (info[2] & ((int)1 << 22)) != 0;

    HW_AVX512_F = (info[1] & ((int)1 << 16)) != 0;
    HW_AVX512_CD = (info[1] & ((int)1 << 28)) != 0;
    HW_AVX512_PF = (info[1] & ((int)1 << 26)) != 0;
    HW_AVX512_ER = (info[1] & ((int)1 << 27)) != 0;

    HW_AVX512_VL = (info[1] & ((int)1 << 31)) != 0;
    HW_AVX512_BW = (info[1] & ((int)1 << 30)) != 0;
    HW_AVX512_DQ = (info[1] & ((int)1 << 17)) != 0;

    HW_AVX512_IFMA = (info[1] & ((int)1 << 21)) != 0;
    HW_AVX512_VBMI = (info[2] & ((int)1 << 1)) != 0;

    HW_AVX512_VPOPCNTDQ = (info[2] & ((int)1 << 14)) != 0;
    HW_AVX512_4FMAPS = (info[3] & ((int)1 << 2)) != 0;
    HW_AVX512_4VNNIW = (info[3] & ((int)1 << 3)) != 0;

    HW_AVX512_VNNI = (info[2] & ((int)1 << 11)) != 0;

    HW_AVX512_VBMI2 = (info[2] & ((int)1 << 6)) != 0;
    HW_GFNI = (info[2] & ((int)1 << 8)) != 0;
    HW_VAES = (info[2] & ((int)1 << 9)) != 0;
    HW_AVX512_VPCLMUL = (info[2] & ((int)1 << 10)) != 0;
    HW_AVX512_BITALG = (info[2] & ((int)1 << 12)) != 0;


    cpuid(info, 0x00000007, 1);
    HW_AVX512_BF16 = (info[0] & ((int)1 << 5)) != 0;
  }
  if (nExIds >= 0x80000001)
  {
    cpuid(info, 0x80000001, 0);
    HW_x64 = (info[3] & ((int)1 << 29)) != 0;
    HW_ABM = (info[2] & ((int)1 << 5)) != 0;
    HW_SSE4a = (info[2] & ((int)1 << 6)) != 0;
    HW_FMA4 = (info[2] & ((int)1 << 16)) != 0;
    HW_XOP = (info[2] & ((int)1 << 11)) != 0;
    HW_PREFETCHW = (info[2] & ((int)1 << 8)) != 0;
  }
}

#else

void ezCpuFeatures::Detect()
{
}

#endif

EZ_STATICLINK_FILE(Foundation, Foundation_System_Implementation_SystemInformation);
