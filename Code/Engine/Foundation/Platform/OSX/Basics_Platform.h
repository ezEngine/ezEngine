#pragma once

#undef EZ_PLATFORM_OSX
#define EZ_PLATFORM_OSX EZ_ON

#undef EZ_PLATFORM_LITTLE_ENDIAN
#define EZ_PLATFORM_LITTLE_ENDIAN EZ_ON

#undef EZ_PLATFORM_PATH_SEPARATOR
#define EZ_PLATFORM_PATH_SEPARATOR '/'

#include <cstdio>
#include <pthread.h>
#include <sys/malloc.h>
#include <sys/time.h>

// unset common macros
#undef min
#undef max
