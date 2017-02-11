#pragma once

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)
  #error "uwp util header should only be included in UWP builds!"
#endif

#include <Foundation/Types/Types.h>

#include <windows.foundation.h>
#include <wrl/wrappers/corewrappers.h>
#include <wrl/client.h>
#include <wrl/implements.h>

using namespace Microsoft::WRL::Wrappers;
using namespace Microsoft::WRL;

inline ezResult ezToResult(ezResult result)
{
  return result;
}

inline ezResult ezToResult(HRESULT result)
{
  return SUCCEEDED(result) ? EZ_SUCCESS : EZ_FAILURE;
}

#undef EZ_SUCCEED_OR_RETURN
#define EZ_SUCCEED_OR_RETURN(code) \
  do { auto s = (code); if (ezToResult(s).Failed()) return EZ_FAILURE; } while(false)

#define EZ_SUCCEED_OR_RETURN_HRESULT(x) \
  do { HRESULT h = (x); if(FAILED(h)) return h; } while(false)
