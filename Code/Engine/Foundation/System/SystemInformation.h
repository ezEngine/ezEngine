
#pragma once

/// \brief The system configuration class encapsulates information about the system the application is running on.
///
/// Retrieve the system configuration by using ezSystemInformation::Get(). If you use the system configuration in startup code
/// make sure to add the correct dependency to the system "SystemInformation" in "Foundation".
class EZ_FOUNDATION_DLL ezSystemInformation
{
public:

  /// \brief Returns the installed physical memory in bytes
  inline ezUInt64 GetInstalledMainMemory() const
  {
    return m_uiInstalledMainMemory;
  }

  /// \brief Returns the size of a memory page in bytes
  inline ezUInt32 GetMemoryPageSize() const
  {
    return m_uiMemoryPageSize;
  }

  /// \brief Returns the CPU core count of the system.
  inline ezUInt32 GetCPUCoreCount() const
  {
    return m_uiCPUCoreCount;
  }

  /// \brief Returns true if the process is currently running on a 64-bit OS.
  inline bool Is64BitOS() const
  {
    return m_b64BitOS;
  }

  inline const char* GetPlatformName() const
  {
    return m_szPlatformName;
  }

  inline const char* GetHostName() const
  {
    return m_sHostName;
  }

  inline const char* GetBuildConfiguration() const
  {
    return m_szBuildConfiguration;
  }
public:

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
  const char* m_szPlatformName;
  const char* m_szBuildConfiguration;
  char m_sHostName[256];
  bool m_b64BitOS;
  bool m_bIsInitialized;


  static void Initialize();

  static ezSystemInformation s_SystemInformation;

};

