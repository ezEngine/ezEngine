#pragma once

#if EZ_DISABLED(EZ_PLATFORM_OSX)
  #error "This header should only be included on OSX"
#endif

#include <sys/malloc.h>
#include <pthread.h>
#include <cstdio>
#include <sys/time.h>

// unset common macros
#undef min
#undef max

/// \todo Detect / differentiate between GCC / CLANG?
#include <Foundation/Basics/Compiler/GCC/GCC.h>

#undef EZ_PLATFORM_LITTLE_ENDIAN
#define EZ_PLATFORM_LITTLE_ENDIAN EZ_ON

#if __x86_64__
  #undef EZ_PLATFORM_64BIT
  #define EZ_PLATFORM_64BIT EZ_ON
#else
  #undef EZ_PLATFORM_32BIT
  #define EZ_PLATFORM_32BIT EZ_ON
#endif

