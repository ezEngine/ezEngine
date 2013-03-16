#pragma once

#if defined(_WINDOWS)
  #define EZ_PLATFORM_WINDOWS 1

#elif defined(__APPLE__) && defined(__MACH__)
  #include <TargetConditionals.h>

  #if TARGET_OS_MAC == 1
    #define EZ_PLATFORM_OSX 1
  #elif TARGET_OS_IPHONE == 1 || TARGET_IPHONE_SIMULATOR == 1
    #define EZ_PLATFORM_IOS 1
  #endif

//#elif defined(...)
//  #define EZ_PLATFORM_LINUX 1
#else
  #error "Unknown Platform."
#endif
