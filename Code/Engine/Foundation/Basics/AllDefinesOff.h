#pragma once

/// \file

/// \brief Used in conjunction with EZ_ENABLED and EZ_DISABLED for safe checks. Define something to EZ_ON or EZ_OFF to work with those macros.
#define EZ_ON =

/// \brief Used in conjunction with EZ_ENABLED and EZ_DISABLED for safe checks. Define something to EZ_ON or EZ_OFF to work with those macros.
#define EZ_OFF !

/// \brief Used in conjunction with EZ_ON and EZ_OFF for safe checks. Use #if EZ_ENABLED(x) or #if EZ_DISABLED(x) in conditional compilation.
#define EZ_ENABLED(x) (1 EZ_CONCAT(x, =) 1)

/// \brief Used in conjunction with EZ_ON and EZ_OFF for safe checks. Use #if EZ_ENABLED(x) or #if EZ_DISABLED(x) in conditional compilation.
#define EZ_DISABLED(x) (1 EZ_CONCAT(x, =) 2)

/// \brief Checks whether x AND y are both defined as EZ_ON or EZ_OFF. Usually used to check whether configurations overlap, to issue an error.
#define EZ_IS_NOT_EXCLUSIVE(x, y) ((1 EZ_CONCAT(x, =) 1) == (1 EZ_CONCAT(y, =) 1))



// All the supported Platforms
#define EZ_PLATFORM_WINDOWS EZ_OFF         // enabled for all Windows platforms, both UWP and desktop
#define EZ_PLATFORM_WINDOWS_UWP EZ_OFF     // enabled for UWP apps, together with EZ_PLATFORM_WINDOWS
#define EZ_PLATFORM_WINDOWS_DESKTOP EZ_OFF // enabled for desktop apps, together with EZ_PLATFORM_WINDOWS
#define EZ_PLATFORM_OSX EZ_OFF
#define EZ_PLATFORM_LINUX EZ_OFF
#define EZ_PLATFORM_IOS EZ_OFF
#define EZ_PLATFORM_ANDROID EZ_OFF

// Different Bit OSes
#define EZ_PLATFORM_32BIT EZ_OFF
#define EZ_PLATFORM_64BIT EZ_OFF

// Different CPU architectures
#define EZ_PLATFORM_ARCH_X86 EZ_OFF
#define EZ_PLATFORM_ARCH_ARM EZ_OFF

// Endianess
#define EZ_PLATFORM_LITTLE_ENDIAN EZ_OFF
#define EZ_PLATFORM_BIG_ENDIAN EZ_OFF

// Different Compilers
#define EZ_COMPILER_MSVC EZ_OFF
#define EZ_COMPILER_MSVC_CLANG EZ_OFF // Clang front-end with MSVC CodeGen
#define EZ_COMPILER_MSVC_PURE EZ_OFF  // MSVC front-end and CodeGen, no mixed compilers
#define EZ_COMPILER_CLANG EZ_OFF
#define EZ_COMPILER_GCC EZ_OFF

// How to compile the engine
#define EZ_COMPILE_ENGINE_AS_DLL EZ_OFF
#define EZ_COMPILE_FOR_DEBUG EZ_OFF
#define EZ_COMPILE_FOR_DEVELOPMENT EZ_OFF

// Platform Features
#define EZ_USE_POSIX_FILE_API EZ_OFF
#define EZ_SUPPORTS_FILE_ITERATORS EZ_OFF
#define EZ_SUPPORTS_FILE_STATS EZ_OFF
#define EZ_SUPPORTS_MEMORY_MAPPED_FILE EZ_OFF
#define EZ_SUPPORTS_SHARED_MEMORY EZ_OFF
#define EZ_SUPPORTS_DYNAMIC_PLUGINS EZ_OFF
#define EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS EZ_OFF
#define EZ_SUPPORTS_CASE_INSENSITIVE_PATHS EZ_OFF
#define EZ_SUPPORTS_CRASH_DUMPS EZ_OFF

// Allocators
#define EZ_USE_ALLOCATION_TRACKING EZ_OFF
#define EZ_USE_ALLOCATION_STACK_TRACING EZ_OFF
#define EZ_USE_GUARDED_ALLOCATIONS EZ_OFF

// Other Features
#define EZ_USE_PROFILING EZ_OFF

// Hashed String
/// \brief Ref counting on hashed strings adds the possibility to cleanup unused strings. Since ref counting has a performance overhead it is disabled by default.
#define EZ_HASHED_STRING_REF_COUNTING EZ_OFF

// Math Debug Checks
#define EZ_MATH_CHECK_FOR_NAN EZ_OFF

// SIMD support
#define EZ_SIMD_IMPLEMENTATION_FPU 1
#define EZ_SIMD_IMPLEMENTATION_SSE 2

#define EZ_SIMD_IMPLEMENTATION 0

