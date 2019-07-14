#include <FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#include <Foundation/Application/Implementation/Uwp/ApplicationEntryPoint_uwp.h>
#include <roapi.h>

namespace ezApplicationDetails
{
  ezResult InitializeWinrt()
  {
    HRESULT result = RoInitialize(RO_INIT_MULTITHREADED);
    if (FAILED(result))
    {
      printf("Failed to init WinRT: %i", result);
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  void UninitializeWinrt()
  {
    RoUninitialize();
  }
}
#endif
