#pragma once

// Enables profiling features.
#define EZ_PROFILING_ENABLED 1

// Enables some basic checks, even in release builds.
#define EZ_COMPILE_FOR_DEVELOPMENT 1

// Compile the Engine as a DLL or a static library.
//#define EZ_COMPILE_ENGINE_AS_DLL 1

#if EZ_PLATFORM_WINDOWS
  // Always compile as a DLL on Windows
  #define EZ_COMPILE_ENGINE_AS_DLL 1
#else
  // Compile as a static library on all other platforms.
  #define EZ_COMPILE_ENGINE_AS_DLL 0
#endif
