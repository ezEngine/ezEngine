#pragma once

#ifdef BUILDSYSTEM_BUILDING_FOUNDATION_LIB
#  if BUILDSYSTEM_COMPILE_ENGINE_AS_DLL && EZ_DISABLED(EZ_COMPILE_ENGINE_AS_DLL)
#    error "The Buildsystem is configured to build the Engine as a shared library, but EZ_COMPILE_ENGINE_AS_DLL is not defined in UserConfig.h"
#  endif
#  if !BUILDSYSTEM_COMPILE_ENGINE_AS_DLL && EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#    error "The Buildsystem is configured to build the Engine as a static library, but EZ_COMPILE_ENGINE_AS_DLL is defined in UserConfig.h"
#  endif
#endif

#ifndef NULL
#  error "NULL is not defined."
#endif

#ifndef EZ_FORCE_INLINE
#  error "EZ_FORCE_INLINE is not defined."
#endif

#ifndef EZ_ALWAYS_INLINE
#  error "EZ_ALWAYS_INLINE is not defined."
#endif

#ifndef EZ_ALIGNMENT_OF
#  error "EZ_ALIGNMENT_OF is not defined."
#endif

#if EZ_IS_NOT_EXCLUSIVE(EZ_PLATFORM_32BIT, EZ_PLATFORM_64BIT)
#  error "Platform is not defined as 32 Bit or 64 Bit"
#endif

#ifndef EZ_DEBUG_BREAK
#  error "EZ_DEBUG_BREAK is not defined."
#endif

#ifndef EZ_SOURCE_FUNCTION
#  error "EZ_SOURCE_FUNCTION is not defined."
#endif

#ifndef EZ_SOURCE_FILE
#  error "EZ_SOURCE_FILE is not defined."
#endif

#ifndef EZ_SOURCE_LINE
#  error "EZ_SOURCE_LINE is not defined."
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
