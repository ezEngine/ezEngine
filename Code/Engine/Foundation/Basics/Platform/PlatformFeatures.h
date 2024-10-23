#pragma once

// include the platform specific header
#include <Features_Platform.h>

#ifdef BUILDSYSTEM_ENABLE_GLFW_SUPPORT
#  define EZ_SUPPORTS_GLFW EZ_ON
#else
#  define EZ_SUPPORTS_GLFW EZ_OFF
#endif

// now check that the defines for each feature are set (either to 1 or 0, but they must be defined)

#ifndef EZ_SUPPORTS_FILE_ITERATORS
#  error "EZ_SUPPORTS_FILE_ITERATORS is not defined."
#endif

#ifndef EZ_USE_POSIX_FILE_API
#  error "EZ_USE_POSIX_FILE_API is not defined."
#endif

#ifndef EZ_SUPPORTS_FILE_STATS
#  error "EZ_SUPPORTS_FILE_STATS is not defined."
#endif

#ifndef EZ_SUPPORTS_MEMORY_MAPPED_FILE
#  error "EZ_SUPPORTS_MEMORY_MAPPED_FILE is not defined."
#endif

#ifndef EZ_SUPPORTS_SHARED_MEMORY
#  error "EZ_SUPPORTS_SHARED_MEMORY is not defined."
#endif

#ifndef EZ_SUPPORTS_DYNAMIC_PLUGINS
#  error "EZ_SUPPORTS_DYNAMIC_PLUGINS is not defined."
#endif

#ifndef EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#  error "EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS is not defined."
#endif

#ifndef EZ_SUPPORTS_CASE_INSENSITIVE_PATHS
#  error "EZ_SUPPORTS_CASE_INSENSITIVE_PATHS is not defined."
#endif

#ifndef EZ_SUPPORTS_LONG_PATHS
#  error "EZ_SUPPORTS_LONG_PATHS is not defined."
#endif

#if EZ_IS_NOT_EXCLUSIVE(EZ_PLATFORM_32BIT, EZ_PLATFORM_64BIT)
#  error "Platform is not defined as 32 Bit or 64 Bit"
#endif

#if EZ_IS_NOT_EXCLUSIVE(EZ_PLATFORM_LITTLE_ENDIAN, EZ_PLATFORM_BIG_ENDIAN)
#  error "Endianess is not correctly defined."
#endif

#ifndef EZ_MATH_CHECK_FOR_NAN
#  error "EZ_MATH_CHECK_FOR_NAN is not defined."
#endif

#if EZ_IS_NOT_EXCLUSIVE3(EZ_PLATFORM_ARCH_X86, EZ_PLATFORM_ARCH_ARM, EZ_PLATFORM_ARCH_WEB)
#  error "Platform architecture is not correctly defined."
#endif

#if !defined(EZ_SIMD_IMPLEMENTATION) || (EZ_SIMD_IMPLEMENTATION == 0)
#  error "EZ_SIMD_IMPLEMENTATION is not correctly defined."
#endif
