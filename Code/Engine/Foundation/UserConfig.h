#pragma once

/// \file

/// Global settings for how to to compile EZ.
/// Modify these settings as you needed in your project.


#ifdef BUILDSYSTEM_COMPILE_ENGINE_AS_DLL
#  undef EZ_COMPILE_ENGINE_AS_DLL
#  define EZ_COMPILE_ENGINE_AS_DLL EZ_ON
#else
#  undef EZ_COMPILE_ENGINE_AS_DLL
#  define EZ_COMPILE_ENGINE_AS_DLL EZ_OFF
#endif

#if defined(BUILDSYSTEM_BUILDTYPE_Shipping)

// Development checks like assert.
#  undef EZ_COMPILE_FOR_DEVELOPMENT
#  define EZ_COMPILE_FOR_DEVELOPMENT EZ_OFF

// Performance profiling features
#  undef EZ_USE_PROFILING
#  define EZ_USE_PROFILING EZ_OFF

// Tracking of memory allocations.
#  undef EZ_ALLOC_TRACKING_DEFAULT
#  define EZ_ALLOC_TRACKING_DEFAULT ezAllocatorTrackingMode::Nothing

#else

// Development checks like assert.
#  undef EZ_COMPILE_FOR_DEVELOPMENT
#  define EZ_COMPILE_FOR_DEVELOPMENT EZ_ON

// Performance profiling features
#  undef EZ_USE_PROFILING
#  define EZ_USE_PROFILING EZ_ON

// Tracking of memory allocations.
#  undef EZ_ALLOC_TRACKING_DEFAULT

#  if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#    define EZ_ALLOC_TRACKING_DEFAULT ezAllocatorTrackingMode::AllocationStatsIgnoreLeaks
#  else
#    define EZ_ALLOC_TRACKING_DEFAULT ezAllocatorTrackingMode::AllocationStatsAndStacktraces
#  endif

#endif

#if defined(BUILDSYSTEM_BUILDTYPE_Debug)
#  undef EZ_MATH_CHECK_FOR_NAN
#  define EZ_MATH_CHECK_FOR_NAN EZ_ON
#  undef EZ_USE_STRING_VALIDATION
#  define EZ_USE_STRING_VALIDATION EZ_ON
#endif


/// Whether game objects compute and store their velocity since the last frame (increases object size)
#define EZ_GAMEOBJECT_VELOCITY EZ_ON
