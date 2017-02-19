#pragma once

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)
  #error "uwp util header should only be included in UWP builds!"
#endif

#include <Foundation/Types/Types.h>
#include <Foundation/Basics/Platform/Win/HResultUtils.h>

#include <windows.foundation.h>
#include <wrl/wrappers/corewrappers.h>
#include <wrl/client.h>
#include <wrl/implements.h>

using namespace Microsoft::WRL::Wrappers;
using namespace Microsoft::WRL;
