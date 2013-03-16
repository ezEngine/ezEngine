#pragma once

#if EZ_PLATFORM_WINDOWS
  #include <Foundation/Threading/Implementation/Win/ThreadingDeclarations_win.h>
#elif EZ_PLATFORM_OSX
  #include <Foundation/Threading/Implementation/Posix/ThreadingDeclarations_posix.h>
#else
  #error "Unknown Platform."
#endif