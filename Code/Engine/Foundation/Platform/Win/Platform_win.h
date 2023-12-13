#pragma once

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS)
#  error "This header should only be included on windows platforms"
#endif

#ifdef _WIN64
#  undef EZ_PLATFORM_64BIT
#  define EZ_PLATFORM_64BIT EZ_ON
#else
#  undef EZ_PLATFORM_32BIT
#  define EZ_PLATFORM_32BIT EZ_ON
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <winapifamily.h>

#undef EZ_PLATFORM_WINDOWS_UWP
#undef EZ_PLATFORM_WINDOWS_DESKTOP

// Distinguish between Windows desktop and Windows UWP.
#if WINAPI_FAMILY == WINAPI_FAMILY_APP
#  define EZ_PLATFORM_WINDOWS_UWP EZ_ON
#  define EZ_PLATFORM_WINDOWS_DESKTOP EZ_OFF
#else
#  define EZ_PLATFORM_WINDOWS_UWP EZ_OFF
#  define EZ_PLATFORM_WINDOWS_DESKTOP EZ_ON
#endif

#ifndef NULL
#  define NULL 0
#endif

#undef EZ_PLATFORM_LITTLE_ENDIAN
#define EZ_PLATFORM_LITTLE_ENDIAN EZ_ON

#include <Foundation/Basics/Compiler/Clang/Clang.h>
#include <Foundation/Basics/Compiler/GCC/GCC.h>
#include <Foundation/Basics/Compiler/MSVC/MSVC.h>
