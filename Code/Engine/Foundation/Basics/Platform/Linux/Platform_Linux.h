#pragma once

#if EZ_DISABLED(EZ_PLATFORM_LINUX) && EZ_DISABLED(EZ_PLATFORM_ANDROID)
#  error "This header should only be included on Linux"
#endif

#include <cstdio>
#include <malloc.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>

// unset common macros
#ifdef min
#  undef min
#endif
#ifdef max
#  undef max
#endif

#undef EZ_PLATFORM_LITTLE_ENDIAN
#define EZ_PLATFORM_LITTLE_ENDIAN EZ_ON
