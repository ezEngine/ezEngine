
// Deactivate Doxygen document generation for the following block.
/// \cond

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

// Taken and modified from: http://msdn.microsoft.com/en-us/library/windows/desktop/ms684139(v=vs.85).aspx (Documentation for IsWow64Process on MSDN)
BOOL IsWow64()
{
  BOOL bIsWow64 = FALSE;

  // IsWow64Process is not available on all supported versions of Windows.
  // Use GetModuleHandle to get a handle to the DLL that contains the function
  // and GetProcAddress to get a pointer to the function if available.

  HMODULE hModule = GetModuleHandle(TEXT("kernel32"));
  EZ_ASSERT_RELEASE(hModule != nullptr, "Could not find Kernel32 DLL.");

  LPFN_ISWOW64PROCESS pfnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(hModule,"IsWow64Process");

  if (nullptr != pfnIsWow64Process)
  {
    if (!pfnIsWow64Process(GetCurrentProcess(),&bIsWow64))
    {
      // Since we can't check let's assume 32bit for now
      // (function not supported on XP pre SP2 and most XP installs are 32 bit)
      return false;
    }
  }

  return bIsWow64;
}


// Helper function to detect a 64-bit Windows
bool Is64BitWindows()
{
#if defined(_WIN64)
 return true;  // 64-bit programs run only on Win64 (although if we get to Win128 this will be wrong probably)
#elif defined(_WIN32)
 // 32-bit programs run on both 32-bit and 64-bit Windows
 return IsWow64() == TRUE;
#else
 return false; // Win64 does not support Win16
#endif
}

/// \endcond

void ezSystemInformation::Initialize()
{
  // Get system information via various APIs
  SYSTEM_INFO sysInfo;
  ZeroMemory(&sysInfo, sizeof(sysInfo));
  GetNativeSystemInfo(&sysInfo);

  s_SystemInformation.m_uiCPUCoreCount = sysInfo.dwNumberOfProcessors;
  s_SystemInformation.m_uiMemoryPageSize = sysInfo.dwPageSize;

  MEMORYSTATUSEX memStatus;
  ZeroMemory(&memStatus, sizeof(memStatus));
  memStatus.dwLength = sizeof(memStatus);
  GlobalMemoryStatusEx(&memStatus);

  s_SystemInformation.m_uiInstalledMainMemory = memStatus.ullTotalPhys;
  s_SystemInformation.m_b64BitOS = Is64BitWindows();
  s_SystemInformation.m_szPlatformName = "Windows";

#if defined BUILDSYSTEM_CONFIGURATION
  s_SystemInformation.m_szBuildConfiguration = BUILDSYSTEM_CONFIGURATION;
#else
  s_SystemInformation.m_szBuildConfiguration = "undefined";
#endif

  //  Get host name
  DWORD bufCharCount = sizeof(s_SystemInformation.m_sHostName);
  if (!GetComputerName(s_SystemInformation.m_sHostName, &bufCharCount))
  {
    strcpy(s_SystemInformation.m_sHostName, "");
  }

}

