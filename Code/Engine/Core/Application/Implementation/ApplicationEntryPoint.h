
#pragma once

#include <Core/Basics.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

  #include <Core/Application/Implementation/Win/ApplicationEntryPoint_win.h>

#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)

  #include <Core/Application/Implementation/Posix/ApplicationEntryPoint_posix.h>

#else
  #error "Missing definition of platform specific entry point!"
#endif

