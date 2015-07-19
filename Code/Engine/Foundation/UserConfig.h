#pragma once

/// \file

#if !defined(BUILDSYSTEM_IGNORE_USERCONFIG_HEADER)

  /// Modify these settings as you wish.
  /// However, they will only take effect when the CMake build setting 'EZ_USE_USERCONFIG_HEADER' is disabled
  /// Prefer to use this file to configure these settings, as this will also work when you build your application with another
  /// build-system (and thus the pre-defined defines such as BUILDSYSTEM_COMPILE_FOR_DEVELOPMENT are not automatically there as well).

  // Enables profiling features.
  #undef EZ_USE_PROFILING
  #define EZ_USE_PROFILING EZ_ON

  // Enables some basic checks, even in release builds.
  #undef EZ_COMPILE_FOR_DEVELOPMENT
  #define EZ_COMPILE_FOR_DEVELOPMENT EZ_ON

  // Compile the Engine as a DLL or a static library.
  #if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    // Always compile as a DLL on Windows
    #undef EZ_COMPILE_ENGINE_AS_DLL
    #define EZ_COMPILE_ENGINE_AS_DLL EZ_ON
  #else
    // Compile as a static library on all other platforms.
    #undef EZ_COMPILE_ENGINE_AS_DLL
    #define EZ_COMPILE_ENGINE_AS_DLL EZ_OFF
  #endif

  #undef EZ_USE_ALLOCATION_STACK_TRACING
  #define EZ_USE_ALLOCATION_STACK_TRACING EZ_ON

#else

  // When the CMake build files are used pass through the CMake configuration

  #ifdef BUILDSYSTEM_USE_PROFILING
    #undef EZ_USE_PROFILING
    #define EZ_USE_PROFILING EZ_ON
  #else
    #undef EZ_USE_PROFILING
    #define EZ_USE_PROFILING EZ_OFF
  #endif

  #ifdef BUILDSYSTEM_COMPILE_FOR_DEVELOPMENT
    #undef EZ_COMPILE_FOR_DEVELOPMENT
    #define EZ_COMPILE_FOR_DEVELOPMENT EZ_ON
  #else
    #undef EZ_COMPILE_FOR_DEVELOPMENT
    #define EZ_COMPILE_FOR_DEVELOPMENT EZ_OFF
  #endif

  #ifdef BUILDSYSTEM_USE_ALLOCATION_STACK_TRACING
    #undef EZ_USE_ALLOCATION_STACK_TRACING
    #define EZ_USE_ALLOCATION_STACK_TRACING EZ_ON
  #else
    #undef EZ_USE_ALLOCATION_STACK_TRACING
    #define EZ_USE_ALLOCATION_STACK_TRACING EZ_OFF
  #endif

  #ifdef BUILDSYSTEM_COMPILE_ENGINE_AS_DLL
    #undef EZ_COMPILE_ENGINE_AS_DLL
    #define EZ_COMPILE_ENGINE_AS_DLL EZ_ON
  #else
    #undef EZ_COMPILE_ENGINE_AS_DLL
    #define EZ_COMPILE_ENGINE_AS_DLL EZ_OFF
  #endif

#endif 