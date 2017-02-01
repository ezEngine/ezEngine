
// Deactivate Doxygen document generation for the following block.
/// \cond

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

// TODO: Put these fundamental things into a UWP helper header.
////////////////////////

// For ComPtr
#include <wrl/client.h>
// For HString, HStringReference and co.
#include <wrl/wrappers/corewrappers.h>
// For Windows::Foundation::GetActivationFactory and similar.
#include <windows.foundation.h>

// Don't want to type Microsoft::WRL::ComPtr all the time.
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

// To get from wchar_t (from HString) to char*
#include <Foundation/Strings/StringConversion.h>

////////////////////////


#include <windows.networking.connectivity.h>

#endif

// Helper function to detect a 64-bit Windows
bool Is64BitWindows()
{
#if defined(_WIN64)
 return true;  // 64-bit programs run only on Win64 (although if we get to Win128 this will be wrong probably)
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

void ezSystemInformation::Initialize()
{
  if (s_SystemInformation.m_bIsInitialized)
    return;

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
  strcpy(s_SystemInformation.m_sHostName, "");

  using namespace ABI::Windows::Networking::Connectivity;
  using namespace ABI::Windows::Networking;
  ComPtr<INetworkInformationStatics> networkInformation;
  if (SUCCEEDED(ABI::Windows::Foundation::GetActivationFactory(HStringReference(InterfaceName_Windows_Networking_Connectivity_INetworkInformationStatics).Get(), &networkInformation)))
  {
    ComPtr<ABI::Windows::Foundation::Collections::IVectorView<HostName*>> hostNames;
    if (SUCCEEDED(networkInformation->GetHostNames(&hostNames)))
    {
      unsigned int numHostNames = 0;
      if (SUCCEEDED(hostNames->get_Size(&numHostNames)))
      {
        for (unsigned int i = 0; i < numHostNames; ++i)
        {
          ComPtr<IHostName> hostName;
          if (FAILED(hostNames->GetAt(i, &hostName)))
            continue;

          HostNameType hostNameType;
          if (FAILED(hostName->get_Type(&hostNameType)))
            continue;

          if (hostNameType == HostNameType_DomainName)
          {
            HString name;
            if (FAILED(hostName->get_CanonicalName(name.GetAddressOf())))
              continue;

            unsigned int stringLen = 0;
            const wchar_t* rawName = name.GetRawBuffer(&stringLen);
            ezStringUtils::Copy(s_SystemInformation.m_sHostName, sizeof(s_SystemInformation.m_sHostName), ezStringUtf8(rawName).GetData());
            break;
          }
        }
      }
    }
  }

#else
  DWORD bufCharCount = sizeof(s_SystemInformation.m_sHostName);
  if (!GetComputerName(s_SystemInformation.m_sHostName, &bufCharCount))
  {
    strcpy(s_SystemInformation.m_sHostName, "");
  }
#endif


  s_SystemInformation.m_bIsInitialized = true;
}

