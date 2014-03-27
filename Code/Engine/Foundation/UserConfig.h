#pragma once

// Enables profiling features.
#undef EZ_USE_PROFILING
#define EZ_USE_PROFILING EZ_ON

#undef EZ_USE_PROFILING_GPA
#define EZ_USE_PROFILING_GPA EZ_ON

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

