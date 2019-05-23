// Deactivate Doxygen document generation for the following block.
/// \cond

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#include <windows.networking.connectivity.h>
#endif

#include <Foundation/Strings/String.h>

// Helper function to detect a 64-bit Windows
bool Is64BitWindows()
{
#if defined(_WIN64)
  return true; // 64-bit programs run only on Win64 (although if we get to Win128 this will be wrong probably)
#elif defined(_WIN32)
  // 32-bit programs run on both 32-bit and 64-bit Windows
  // Note that we used IsWow64Process before which is not available on UWP.
  SYSTEM_INFO info;
  GetNativeSystemInfo(&info);
  // According to documentation: "The processor architecture of the installed operating system."
  return info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64;
#else
  return false; // Win64 does not support Win16
#endif
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
  s_SystemInformation.m_b64BitOS = Is64BitWindows();
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  s_SystemInformation.m_szPlatformName = "Windows - UWP";
#else
  s_SystemInformation.m_szPlatformName = "Windows - Desktop";
#endif

#if defined BUILDSYSTEM_CONFIGURATION
  s_SystemInformation.m_szBuildConfiguration = BUILDSYSTEM_CONFIGURATION;
#else
  s_SystemInformation.m_szBuildConfiguration = "undefined";
#endif

  //  Get host name


#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  using namespace ABI::Windows::Networking::Connectivity;
  using namespace ABI::Windows::Networking;
  ComPtr<INetworkInformationStatics> networkInformation;
  if (SUCCEEDED(ABI::Windows::Foundation::GetActivationFactory(
          HStringReference(RuntimeClass_Windows_Networking_Connectivity_NetworkInformation).Get(), &networkInformation)))
  {
    ComPtr<ABI::Windows::Foundation::Collections::IVectorView<HostName*>> hostNames;
    if (SUCCEEDED(networkInformation->GetHostNames(&hostNames)))
    {
      ezUwpUtils::ezWinRtIterateIVectorView<IHostName*>(hostNames, [](UINT, IHostName* hostName) {
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

        return true;
      });
    }
  }

#else
  DWORD bufCharCount = sizeof(s_SystemInformation.m_sHostName);
  GetComputerNameA(s_SystemInformation.m_sHostName, &bufCharCount);
#endif

  s_SystemInformation.m_bIsInitialized = true;
}

