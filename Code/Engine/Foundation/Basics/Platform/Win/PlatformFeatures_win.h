#pragma once

/// \file

/// If set to 1, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#undef EZ_USE_POSIX_FILE_API

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#  define EZ_USE_POSIX_FILE_API EZ_ON
#else
#  define EZ_USE_POSIX_FILE_API EZ_OFF
#endif

/// Iterating through the file system is supported
#undef EZ_SUPPORTS_FILE_ITERATORS

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#  define EZ_SUPPORTS_FILE_ITERATORS EZ_OFF
#else
#  define EZ_SUPPORTS_FILE_ITERATORS EZ_ON
#endif

/// Getting the stats of a file (modification times etc.) is supported.
#undef EZ_SUPPORTS_FILE_STATS
#define EZ_SUPPORTS_FILE_STATS EZ_ON

/// Whether dynamic plugins (through DLLs loaded/unloaded at runtime) are supported
#undef EZ_SUPPORTS_DYNAMIC_PLUGINS
#define EZ_SUPPORTS_DYNAMIC_PLUGINS EZ_ON

/// Whether applications can access any file (not sandboxed)
#undef EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#  define EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS EZ_OFF
#else
#  define EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS EZ_ON
#endif

/// Whether file accesses can be done through paths that do not match exact casing
#undef EZ_SUPPORTS_CASE_INSENSITIVE_PATHS
#define EZ_SUPPORTS_CASE_INSENSITIVE_PATHS EZ_ON

/// Whether starting other processes is supported.
#undef EZ_SUPPORTS_PROCESSES
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#  define EZ_SUPPORTS_PROCESSES EZ_OFF
#else
#  define EZ_SUPPORTS_PROCESSES EZ_ON
#endif

// SIMD support
#undef EZ_SIMD_IMPLEMENTATION

#if EZ_ENABLED(EZ_PLATFORM_ARCH_X86)
#  define EZ_SIMD_IMPLEMENTATION EZ_SIMD_IMPLEMENTATION_SSE
#elif EZ_ENABLED(EZ_PLATFORM_ARCH_ARM)
#  define EZ_SIMD_IMPLEMENTATION EZ_SIMD_IMPLEMENTATION_FPU
#else
#  error "Unknown architecture."
#endif

