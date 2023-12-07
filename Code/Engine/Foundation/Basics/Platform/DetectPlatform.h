#pragma once

// clang-format off
#define EZ_PLATFORM_INCLUDE(name) EZ_PP_STRINGIFY(EZ_PP_IDENTITY(name)EZ_PP_IDENTITY(_)EZ_PP_IDENTITY(BUILDSYSTEM_PLATFORM_SHORTNAME)EZ_PP_IDENTITY(.h))
// clang-format on

#if defined(_WINDOWS) || defined(_WIN32)
#  undef EZ_PLATFORM_WINDOWS
#  define EZ_PLATFORM_WINDOWS EZ_ON

// further distinction between desktop, UWP etc. is done in Platform_win.h

#elif defined(__APPLE__) && defined(__MACH__)
#  include <TargetConditionals.h>

#  if TARGET_OS_MAC == 1
#    undef EZ_PLATFORM_OSX
#    define EZ_PLATFORM_OSX EZ_ON
#  elif TARGET_OS_IPHONE == 1 || TARGET_IPHONE_SIMULATOR == 1
#    undef EZ_PLATFORM_IOS
#    define EZ_PLATFORM_IOS EZ_ON
#  endif

#elif defined(ANDROID)

#  undef EZ_PLATFORM_ANDROID
#  define EZ_PLATFORM_ANDROID EZ_ON

#elif defined(__linux)

#  undef EZ_PLATFORM_LINUX
#  define EZ_PLATFORM_LINUX EZ_ON

//#elif defined(...)
//  #undef EZ_PLATFORM_LINUX
//  #define EZ_PLATFORM_LINUX EZ_ON
#else
#  error "Unknown Platform."
#endif
