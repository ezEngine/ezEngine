#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#  include <Foundation/Platform/UWP/ApplicationEntryPoint_Platform.h>
#  include <roapi.h>

namespace ezApplicationDetails
{
  ezResult InitializeWinrt()
  {
    HRESULT result = RoInitialize(RO_INIT_MULTITHREADED);
    if (FAILED(result))
    {
      ezLog::Printf("Failed to init WinRT: %i", result);
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  void UninitializeWinrt()
  {
    RoUninitialize();
  }
} // namespace ezApplicationDetails
#endif
