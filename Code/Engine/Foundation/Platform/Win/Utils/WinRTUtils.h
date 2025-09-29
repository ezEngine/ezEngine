#pragma once

#include <Foundation/Basics.h>

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS)
#  error "WinRT util header should only be included in Windows builds!"
#endif

// For 10.0.10240, there are circular includes in some headers, fix by including first:
#include <corecrt.h>

// Only include a subset that is C++ conformant
#include <wrl/def.h>
#include <wrl/ftm.h>
#include <wrl/internal.h>
#include <wrl/module.h>
#include <wrl/wrappers/corewrappers.h>

#include <windows.devices.enumeration.h>
#include <windows.foundation.collections.h>
#include <windows.foundation.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
EZ_MSVC_ANALYSIS_WARNING_DISABLE(4530);
