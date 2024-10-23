#pragma once

#undef EZ_PLATFORM_LINUX
#define EZ_PLATFORM_LINUX EZ_ON

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
