#pragma once

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)
  #error "uwp util header should only be included in UWP builds!"
#endif

#include <windows.foundation.h>
#include <wrl/wrappers/corewrappers.h>
#include <wrl/client.h>
#include <wrl/implements.h>

using namespace Microsoft::WRL::Wrappers;
using namespace Microsoft::WRL;

#define EZ_RET_FAILED_HRESULT(x) do { HRESULT h = (x); if(FAILED(h)) return h; } while(false)
#define EZ_RET_FAIL_HRESULT(x) do { HRESULT h = (x); if(FAILED(h)) return EZ_FAILURE; } while(false)
