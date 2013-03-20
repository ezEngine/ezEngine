#pragma once

#ifndef EZ_PLATFORM_OSX
  #error "This header should only be included on OSX"
#endif

#include <sys/malloc.h>
#include <pthread.h>
#include <cstdio>
#include <sys/time.h>

// unset common macros
#undef min
#undef max


typedef void* ezModuleHandle;
//typedef HWND ezWindowHandle;
typedef FILE* ezFileHandle;
//typedef OVERLAPPED ezAsyncIOHandle;
typedef pthread_t ezThreadId;
typedef pthread_t ezThreadHandle;
typedef pthread_mutex_t ezMutexHandle;
typedef timeval ezHighPrecisionTimeValue;
typedef void* (*ezOSThreadEntryPoint)(void* pThreadParameter);
typedef pthread_key_t ezThreadLocalStorageKey;

/// \todo Detect / differentiate between GCC / CLANG?
#include <Foundation/Basics/Compiler/GCC/GCC.h>

#define EZ_LITTLE_ENDIAN

#if __x86_64__
  #define EZ_PLATFORM_64BIT 1
#else
  #define EZ_PLATFORM_32BIT 1
#endif