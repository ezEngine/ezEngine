#pragma once

// This should be defined by the compiler specific header
#ifdef NULL
  #undef NULL
#endif

#define EZ_ON =
#define EZ_OFF !
#define EZ_ENABLED(x) (1 ##x= 1)
#define EZ_DISABLED(x) (1 ##x= 2)
#define EZ_IS_NOT_EXCLUSIVE(x, y) ((1 ##x= 1) == (1 ##y= 1))



// All the supported Platforms
#define EZ_PLATFORM_WINDOWS EZ_OFF
#define EZ_PLATFORM_OSX EZ_OFF
#define EZ_PLATFORM_LINUX EZ_OFF
#define EZ_PLATFORM_IOS EZ_OFF

// Different Bit OSes
#define EZ_PLATFORM_32BIT EZ_OFF
#define EZ_PLATFORM_64BIT EZ_OFF

// Endianess
#define EZ_PLATFORM_LITTLE_ENDIAN EZ_OFF
#define EZ_PLATFORM_BIG_ENDIAN EZ_OFF

// C++ Version
#define EZ_SUPPORTS_CPP11 EZ_OFF

// Windows Compiler
#define EZ_COMPILER_MSVC EZ_OFF

// How to compile the engine
#define EZ_COMPILE_ENGINE_AS_DLL EZ_OFF
#define EZ_COMPILE_FOR_DEBUG EZ_OFF
#define EZ_COMPILE_FOR_DEVELOPMENT OFF

// Platform Features
#define EZ_USE_POSIX_FILE_API EZ_OFF
#define EZ_SUPPORTS_FILE_ITERATORS EZ_OFF
#define EZ_SUPPORTS_FILE_STATS EZ_OFF


// Allocators
#define EZ_USE_GUARDED_ALLOCATOR EZ_OFF
#define EZ_USE_TRACE_ALLOCATOR EZ_OFF


// Other Features
#define EZ_USE_PROFILING EZ_OFF