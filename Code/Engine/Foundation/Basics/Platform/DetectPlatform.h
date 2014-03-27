#pragma once

#if defined(_WINDOWS)
  #undef EZ_PLATFORM_WINDOWS
  #define EZ_PLATFORM_WINDOWS EZ_ON

#elif defined(__APPLE__) && defined(__MACH__)
  #include <TargetConditionals.h>

  #if TARGET_OS_MAC == 1
    #undef EZ_PLATFORM_OSX
    #define EZ_PLATFORM_OSX EZ_ON
  #elif TARGET_OS_IPHONE == 1 || TARGET_IPHONE_SIMULATOR == 1
    #undef EZ_PLATFORM_IOS
    #define EZ_PLATFORM_IOS EZ_ON
  #endif

#elif defined(__linux)

   #undef EZ_PLATFORM_LINUX
   #define EZ_PLATFORM_LINUX EZ_ON

//#elif defined(...)
//  #undef EZ_PLATFORM_LINUX
//  #define EZ_PLATFORM_LINUX EZ_ON
#else
  #error "Unknown Platform."
#endif

