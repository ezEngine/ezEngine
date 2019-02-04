#pragma once

#if EZ_DISABLED(EZ_PLATFORM_OSX)
#  error "This header should only be included on OSX"
#endif

#include <cstdio>
#include <pthread.h>
#include <sys/malloc.h>
#include <sys/time.h>

// unset common macros
#undef min
#undef max

#include <Foundation/Basics/Compiler/Clang/Clang.h>
#include <Foundation/Basics/Compiler/GCC/GCC.h>

#undef EZ_PLATFORM_LITTLE_ENDIAN
#define EZ_PLATFORM_LITTLE_ENDIAN EZ_ON

#if __x86_64__
#  undef EZ_PLATFORM_64BIT
#  define EZ_PLATFORM_64BIT EZ_ON
#else
#  undef EZ_PLATFORM_32BIT
#  define EZ_PLATFORM_32BIT EZ_ON
#endif

#if __arm__
#  undef EZ_PLATFORM_ARCH_ARM
#  define EZ_PLATFORM_ARCH_ARM EZ_ON
#else
#  undef EZ_PLATFORM_ARCH_X86
#  define EZ_PLATFORM_ARCH_X86 EZ_ON
#endif

