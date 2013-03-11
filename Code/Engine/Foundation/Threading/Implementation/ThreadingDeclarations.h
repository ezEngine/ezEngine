#pragma once

#ifdef EZ_PLATFORM_WINDOWS
  #include <Foundation/Threading/Implementation/Win/ThreadingDeclarations_win.h>
#else
  #error "Unknown Platform."
#endif