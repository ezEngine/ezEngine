#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Foundation/System/SystemInformation.h>

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#    include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#    include <windows.networking.connectivity.h>
#  endif

#  include <Foundation/Strings/String.h>

// Helper function to detect a 64-bit Windows
bool Is64BitWindows()
{
#  if defined(_WIN64)
  return true; // 64-bit programs run only on Win64 (although if we get to Win128 this will be wrong probably)
#  elif defined(_WIN32)
  // 32-bit programs run on both 32-bit and 64-bit Windows
  // Note that we used IsWow64Process before which is not available on UWP.
  SYSTEM_INFO info;
  GetNativeSystemInfo(&info);
  // According to documentation: "The processor architecture of the installed operating system."
  return info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64;
#  else
  return false; // Win64 does not support Win16
#  endif
}

/// \endcond

bool ezSystemInformation::IsDebuggerAttached()
{
  return ::IsDebuggerPresent();
}

void ezSystemInformation::Initialize()
{
  if (s_SystemInformation.m_bIsInitialized)
    return;

  s_SystemInformation.m_CpuFeatures.Detect();

  s_SystemInformation.m_sHostName[0] = '\0';

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
  s_SystemInformation.m_bB64BitOS = Is64BitWindows();
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  s_SystemInformation.m_szPlatformName = "Windows - UWP";
#  else
  s_SystemInformation.m_szPlatformName = "Windows - Desktop";
#  endif

#  if defined BUILDSYSTEM_BUILDTYPE
  s_SystemInformation.m_szBuildConfiguration = BUILDSYSTEM_BUILDTYPE;
#  else
  s_SystemInformation.m_szBuildConfiguration = "undefined";
#  endif

  //  Get host name


#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  using namespace ABI::Windows::Networking::Connectivity;
  using namespace ABI::Windows::Networking;
  ComPtr<INetworkInformationStatics> networkInformation;
  if (SUCCEEDED(ABI::Windows::Foundation::GetActivationFactory(
        HStringReference(RuntimeClass_Windows_Networking_Connectivity_NetworkInformation).Get(), &networkInformation)))
  {
    ComPtr<ABI::Windows::Foundation::Collections::IVectorView<HostName*>> hostNames;
    if (SUCCEEDED(networkInformation->GetHostNames(&hostNames)))
    {
      ezUwpUtils::ezWinRtIterateIVectorView<IHostName*>(hostNames, [](UINT, IHostName* hostName)
        {
        HostNameType hostNameType;
        if (FAILED(hostName->get_Type(&hostNameType)))
          return true;

        if (hostNameType == HostNameType_DomainName)
        {
          HString name;
          if (FAILED(hostName->get_CanonicalName(name.GetAddressOf())))
            return true;

          ezStringUtils::Copy(s_SystemInformation.m_sHostName, sizeof(s_SystemInformation.m_sHostName), ezStringUtf8(name).GetData());
          return false;
        }

        return true; });
    }
  }

#  else
  DWORD bufCharCount = sizeof(s_SystemInformation.m_sHostName);
  GetComputerNameA(s_SystemInformation.m_sHostName, &bufCharCount);
#  endif

  s_SystemInformation.m_bIsInitialized = true;
}

ezUInt64 ezSystemInformation::GetAvailableMainMemory() const
{
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  GlobalMemoryStatusEx(&statex);

  return statex.ullAvailPhys;
}

float ezSystemInformation::GetCPUUtilization() const
{
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

  LARGE_INTEGER kernel, user, idle;
  GetSystemTimes((FILETIME*)&idle, (FILETIME*)&kernel, (FILETIME*)&user);

  static thread_local uint64_t lastKernel = 0u, lastIdle = 0u, lastUser = 0u;

  auto kernelTime = kernel.QuadPart - lastKernel;
  auto idleTime = idle.QuadPart - lastIdle;
  auto userTime = user.QuadPart - lastUser;

  lastKernel = kernel.QuadPart;
  lastUser = user.QuadPart;
  lastIdle = idle.QuadPart;

  auto util = static_cast<float>(kernelTime + userTime - idleTime) / (kernelTime + userTime);

  return ezMath::Clamp(util, 0.f, 1.f) * 100.f;

#  else
  EZ_ASSERT_NOT_IMPLEMENTED;
  return 0.0f;
#  endif
}

#  if EZ_ENABLED(EZ_PLATFORM_ARCH_X86)

namespace cpu_x86
{
#    include <Windows.h>
#    include <intrin.h>

  void cpuid(int32_t pOut[4], int32_t eax, int32_t ecx)
  {
    __cpuidex(pOut, eax, ecx);
  }

  uint64_t xgetbv(unsigned int x)
  {
    return _xgetbv(x);
  }

  bool detect_OS_x64()
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
} // namespace cpu_x86

#  else

#  endif

#endif
