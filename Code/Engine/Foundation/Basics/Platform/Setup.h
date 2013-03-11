#pragma once

// This should be defined by the compiler specific header
#ifdef NULL
  #undef NULL
#endif

#ifdef BUILDSYSTEM_COMPILE_FOR_DEBUG
  #define EZ_COMPILE_FOR_DEBUG 1
#endif

#ifdef BUILDSYSTEM_COMPILE_FOR_DEVELOPMENT
  #define EZ_COMPILE_FOR_DEVELOPMENT 1
#endif

#ifdef BUILDSYSTEM_COMPILE_ENGINE_AS_DLL
  #define EZ_COMPILE_ENGINE_AS_DLL 1
#endif

#ifdef BUILDSYSTEM_PLATFORM_WINDOWS
  #define EZ_PLATFORM_WINDOWS 1
#endif
#ifdef BUILDSYSTEM_PLATFORM_LINUX
  #define EZ_PLATFORM_LINUX 1
#endif

#ifdef BUILDSYSTEM_PLATFORM_32BIT
  #define EZ_PLATFORM_32BIT 1
#endif
#ifdef BUILDSYSTEM_PLATFORM_64BIT
  #define EZ_PLATFORM_64BIT 1
#endif

