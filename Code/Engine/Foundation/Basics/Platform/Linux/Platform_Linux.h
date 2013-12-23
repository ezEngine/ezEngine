#pragma once

#if EZ_DISABLED(EZ_PLATFORM_LINUX)
  #error "This header should only be included on Linux"
#endif

#include <malloc.h>
#include <pthread.h>
#include <cstdio>
#include <sys/time.h>
#include <stdarg.h>
#include <unistd.h>

// unset common macros
#undef min
#undef max


typedef void* ezModuleHandle;
typedef FILE* ezFileHandle;
typedef pthread_t ezThreadId;
typedef pthread_t ezThreadHandle;
typedef pthread_mutex_t ezMutexHandle;
typedef timeval ezHighPrecisionTimeValue;
typedef void* (*ezOSThreadEntryPoint)(void* pThreadParameter);
typedef pthread_key_t ezThreadLocalStorageKey;

/// \todo Detect / differentiate between GCC / CLANG?
#include <Foundation/Basics/Compiler/GCC/GCC.h>

#undef EZ_PLATFORM_LITTLE_ENDIAN
#define EZ_PLATFORM_LITTLE_ENDIAN EZ_ON

#if __x86_64
  #undef EZ_PLATFORM_64BIT
  #define EZ_PLATFORM_64BIT EZ_ON
#else
  #undef EZ_PLATFORM_32BIT
  #define EZ_PLATFORM_32BIT EZ_ON
#endif

