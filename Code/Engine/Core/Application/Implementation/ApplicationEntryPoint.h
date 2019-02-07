#pragma once

#include <Core/CoreDLL.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#include <Core/Application/Implementation/Win/ApplicationEntryPoint_win.h>

#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

#include <Core/Application/Implementation/uwp/ApplicationEntryPoint_uwp.h>

#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)

#include <Core/Application/Implementation/Posix/ApplicationEntryPoint_posix.h>

#else
#error "Missing definition of platform specific entry point!"
#endif

