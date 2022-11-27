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
#  undef EZ_USE_ALLOCATION_TRACKING
#  define EZ_USE_ALLOCATION_TRACKING EZ_OFF

// Stack traces for memory allocations.
#  undef EZ_USE_ALLOCATION_STACK_TRACING
#  define EZ_USE_ALLOCATION_STACK_TRACING EZ_OFF

#else

// Development checks like assert.
#  undef EZ_COMPILE_FOR_DEVELOPMENT
#  define EZ_COMPILE_FOR_DEVELOPMENT EZ_ON

// Performance profiling features
#  undef EZ_USE_PROFILING
#  define EZ_USE_PROFILING EZ_ON

// Tracking of memory allocations.
#  undef EZ_USE_ALLOCATION_TRACKING
#  define EZ_USE_ALLOCATION_TRACKING EZ_ON

// Stack traces for memory allocations.
#  undef EZ_USE_ALLOCATION_STACK_TRACING
#  define EZ_USE_ALLOCATION_STACK_TRACING EZ_ON

#endif

/// Whether game objects compute and store their velocity since the last frame (increases object size)
#define EZ_GAMEOBJECT_VELOCITY EZ_ON
