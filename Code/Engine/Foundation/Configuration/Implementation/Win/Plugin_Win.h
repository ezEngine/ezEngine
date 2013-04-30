#include <Foundation/Basics.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

  typedef HMODULE ezPluginModule;

#else
  #error "This file should not have been included."
#endif