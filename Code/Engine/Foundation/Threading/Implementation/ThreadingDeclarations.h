#pragma once

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Threading/Implementation/Win/ThreadingDeclarations_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
  #include <Foundation/Threading/Implementation/Posix/ThreadingDeclarations_posix.h>
#else
  #error "Unknown Platform."
#endif

