#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#  include <Unknwn.h> // Required to interop with IUnknown. Must be included before C++/WinRT headers.

#  include <winrt/Windows.Foundation.Collections.h>
#  include <winrt/Windows.Foundation.h>
#endif

// <StaticLinkUtil::StartHere>
// all include's before this will be left alone and not replaced by the StaticLinkUtil
// all include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library
