#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

#  include <Foundation/System/SystemInformation.h>

#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>

#  include <Foundation/Platform/UWP/Utils/UWPUtils.h>
#  include <Foundation/Strings/String.h>
#  include <windows.networking.connectivity.h>

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
  s_SystemInformation.m_szPlatformName = EZ_PLATFORM_NAME;

#  if defined BUILDSYSTEM_BUILDTYPE
  s_SystemInformation.m_szBuildConfiguration = BUILDSYSTEM_BUILDTYPE;
#  else
  s_SystemInformation.m_szBuildConfiguration = "undefined";
#  endif

  //  Get host name
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
  EZ_ASSERT_NOT_IMPLEMENTED;
  return 0.0f;
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
