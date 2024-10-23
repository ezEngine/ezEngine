#pragma once

#undef EZ_PLATFORM_ANDROID
#define EZ_PLATFORM_ANDROID EZ_ON

#undef EZ_PLATFORM_LITTLE_ENDIAN
#define EZ_PLATFORM_LITTLE_ENDIAN EZ_ON

#undef EZ_PLATFORM_PATH_SEPARATOR
#define EZ_PLATFORM_PATH_SEPARATOR '/'

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
