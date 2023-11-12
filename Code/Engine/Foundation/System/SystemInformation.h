#pragma once

/// \brief Flags that tell you which SIMD features are available on this processor / OS
///
/// Heavily 'inspired' by https://github.com/Mysticial/FeatureDetector
struct ezCpuFeatures
{
#if EZ_ENABLED(EZ_PLATFORM_ARCH_X86)
  //  Vendor
  bool Vendor_AMD = false;
  bool Vendor_Intel = false;

  //  OS Features
  bool OS_x64 = false;
  bool OS_AVX = false;
  bool OS_AVX512 = false;

  //  Misc.
  bool HW_MMX = false;
  bool HW_x64 = false;
  bool HW_ABM = false;
  bool HW_RDRAND = false;
  bool HW_RDSEED = false;
  bool HW_BMI1 = false;
  bool HW_BMI2 = false;
  bool HW_ADX = false;
  bool HW_MPX = false;
  bool HW_PREFETCHW = false;
  bool HW_PREFETCHWT1 = false;
  bool HW_RDPID = false;

  //  SIMD: 128-bit
  bool HW_SSE = false;
  bool HW_SSE2 = false;
  bool HW_SSE3 = false;
  bool HW_SSSE3 = false;
  bool HW_SSE41 = false;
  bool HW_SSE42 = false;
  bool HW_SSE4a = false;
  bool HW_AES = false;
  bool HW_SHA = false;

  //  SIMD: 256-bit
  bool HW_AVX = false;
  bool HW_XOP = false;
  bool HW_FMA3 = false;
  bool HW_FMA4 = false;
  bool HW_AVX2 = false;

  //  SIMD: 512-bit
  bool HW_AVX512_F = false;
  bool HW_AVX512_CD = false;

  //  Knights Landing
  bool HW_AVX512_PF = false;
  bool HW_AVX512_ER = false;

  //  Skylake Purley
  bool HW_AVX512_VL = false;
  bool HW_AVX512_BW = false;
  bool HW_AVX512_DQ = false;

  //  Cannon Lake
  bool HW_AVX512_IFMA = false;
  bool HW_AVX512_VBMI = false;

  //  Knights Mill
  bool HW_AVX512_VPOPCNTDQ = false;
  bool HW_AVX512_4FMAPS = false;
  bool HW_AVX512_4VNNIW = false;

  //  Cascade Lake
  bool HW_AVX512_VNNI = false;

  //  Cooper Lake
  bool HW_AVX512_BF16 = false;

  //  Ice Lake
  bool HW_AVX512_VBMI2 = false;
  bool HW_GFNI = false;
  bool HW_VAES = false;
  bool HW_AVX512_VPCLMUL = false;
  bool HW_AVX512_BITALG = false;

  bool IsAvx1Available() const { return OS_AVX && HW_AVX; }
  bool IsAvx2Available() const { return OS_AVX && HW_AVX2; }
#endif

  void Detect();
};

/// \brief The system configuration class encapsulates information about the system the application is running on.
///
/// Retrieve the system configuration by using ezSystemInformation::Get(). If you use the system configuration in startup code
/// make sure to add the correct dependency to the system "SystemInformation" in "Foundation".
class EZ_FOUNDATION_DLL ezSystemInformation
{
public:
  /// \brief Returns the installed physical memory in bytes
  ezUInt64 GetInstalledMainMemory() const { return m_uiInstalledMainMemory; }

  /// \brief Returns the currently available physical memory
  ezUInt64 GetAvailableMainMemory() const;

  /// \brief Returns the size of a memory page in bytes
  ezUInt32 GetMemoryPageSize() const { return m_uiMemoryPageSize; }

  /// \brief Returns the CPU core count of the system.
  ezUInt32 GetCPUCoreCount() const { return m_uiCPUCoreCount; }

  /// \brief Returns the total utilization of the CPU core in percent
  float GetCPUUtilization() const;

  /// \brief Returns true if the process is currently running on a 64-bit OS.
  bool Is64BitOS() const { return m_bB64BitOS; }

  const char* GetPlatformName() const { return m_szPlatformName; }

  const char* GetHostName() const { return m_sHostName; }

  const char* GetBuildConfiguration() const { return m_szBuildConfiguration; }

  /// \brief Returns a struct that contains detailed information about the available CPU features (SIMD support).
  const ezCpuFeatures& GetCpuFeatures() const { return m_CpuFeatures; }

public:
  /// \brief Returns whether a debugger is currently attached to this process.
  static bool IsDebuggerAttached();

  /// \brief Allows access to the current system configuration.
  static const ezSystemInformation& Get()
  {
    if (!s_SystemInformation.m_bIsInitialized)
      Initialize();

    return s_SystemInformation;
  }

private:
  ezUInt64 m_uiInstalledMainMemory;
  ezUInt32 m_uiMemoryPageSize;
  ezUInt32 m_uiCPUCoreCount;
  const char* m_szPlatformName = nullptr;
  const char* m_szBuildConfiguration = nullptr;
  char m_sHostName[256];
  bool m_bB64BitOS;
  bool m_bIsInitialized;
  ezCpuFeatures m_CpuFeatures;

  static void Initialize();

  static ezSystemInformation s_SystemInformation;
};
