#pragma once

/// \file

#ifdef BUILDSYSTEM_COMPILE_ENGINE_AS_DLL
#  undef EZ_COMPILE_ENGINE_AS_DLL
#  define EZ_COMPILE_ENGINE_AS_DLL EZ_ON
#else
#  undef EZ_COMPILE_ENGINE_AS_DLL
#  define EZ_COMPILE_ENGINE_AS_DLL EZ_OFF
#endif

#ifdef BUILDSYSTEM_USE_PROFILING
#  undef EZ_USE_PROFILING
#  define EZ_USE_PROFILING EZ_ON
#else
#  undef EZ_USE_PROFILING
#  define EZ_USE_PROFILING EZ_OFF
#endif

#ifdef BUILDSYSTEM_COMPILE_FOR_DEVELOPMENT
#  undef EZ_COMPILE_FOR_DEVELOPMENT
#  define EZ_COMPILE_FOR_DEVELOPMENT EZ_ON
#else
#  undef EZ_COMPILE_FOR_DEVELOPMENT
#  define EZ_COMPILE_FOR_DEVELOPMENT EZ_OFF
#endif

#ifdef BUILDSYSTEM_USE_ALLOCATION_TRACKING
#  undef EZ_USE_ALLOCATION_TRACKING
#  define EZ_USE_ALLOCATION_TRACKING EZ_ON
#else
#  undef EZ_USE_ALLOCATION_TRACKING
#  define EZ_USE_ALLOCATION_TRACKING EZ_OFF
#endif

#ifdef BUILDSYSTEM_USE_ALLOCATION_STACK_TRACING
#  undef EZ_USE_ALLOCATION_STACK_TRACING
#  define EZ_USE_ALLOCATION_STACK_TRACING EZ_ON
#else
#  undef EZ_USE_ALLOCATION_STACK_TRACING
#  define EZ_USE_ALLOCATION_STACK_TRACING EZ_OFF
#endif



#if !defined(BUILDSYSTEM_IGNORE_USERCONFIG_HEADER)

/// Modify these settings as you wish.
/// However, they will only take effect when the CMake build setting 'BUILDSYSTEM_IGNORE_USERCONFIG_HEADER' is disabled
/// Prefer to use this file to configure these settings, as this will also work when you build your application with another
/// build-system (and thus the pre-defined defines such as BUILDSYSTEM_COMPILE_FOR_DEVELOPMENT are not automatically there as well).

#  if !defined(BUILDSYSTEM_BUILDTYPE_RELEASE)
// Enables profiling features.
#    undef EZ_USE_PROFILING
#    define EZ_USE_PROFILING EZ_ON

// Enables some development checks like assert.
#    undef EZ_COMPILE_FOR_DEVELOPMENT
#    define EZ_COMPILE_FOR_DEVELOPMENT EZ_ON

// Enables tracking of memory allocations.
#    undef EZ_USE_ALLOCATION_TRACKING
#    define EZ_USE_ALLOCATION_TRACKING EZ_ON

// Enables stack traces for memory allocations.
#    undef EZ_USE_ALLOCATION_STACK_TRACING
#    define EZ_USE_ALLOCATION_STACK_TRACING EZ_ON
#  endif

#  if EZ_DISABLED(EZ_PLATFORM_WINDOWS)
#    if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#      error "DLL builds are not supported on this platform."
#    endif
#  endif

// Uncomment to use guarded allocations. This will use a lot of memory and should only be used in 64bit builds.
//#undef EZ_USE_GUARDED_ALLOCATIONS
//#define EZ_USE_GUARDED_ALLOCATIONS EZ_ON

#endif
