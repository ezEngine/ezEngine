#pragma once

/// \file

#undef EZ_PLATFORM_WINDOWS
#undef EZ_PLATFORM_WINDOWS_UWP
#undef EZ_PLATFORM_WINDOWS_DESKTOP

#define EZ_PLATFORM_WINDOWS EZ_ON
#define EZ_PLATFORM_WINDOWS_UWP EZ_ON
#define EZ_PLATFORM_WINDOWS_DESKTOP EZ_OFF

#undef EZ_PLATFORM_LITTLE_ENDIAN
#define EZ_PLATFORM_LITTLE_ENDIAN EZ_ON

#undef EZ_PLATFORM_PATH_SEPARATOR
#define EZ_PLATFORM_PATH_SEPARATOR '\\'

/// If set to 1, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#undef EZ_USE_POSIX_FILE_API
#define EZ_USE_POSIX_FILE_API EZ_ON

/// Iterating through the file system is supported
#undef EZ_SUPPORTS_FILE_ITERATORS
#define EZ_SUPPORTS_FILE_ITERATORS EZ_OFF

/// Getting the stats of a file (modification times etc.) is supported.
#undef EZ_SUPPORTS_FILE_STATS
#define EZ_SUPPORTS_FILE_STATS EZ_ON

/// Directory watcher is supported on non uwp platforms.
#undef EZ_SUPPORTS_DIRECTORY_WATCHER
#define EZ_SUPPORTS_DIRECTORY_WATCHER EZ_OFF

/// Memory mapping a file is supported.
#undef EZ_SUPPORTS_MEMORY_MAPPED_FILE
#define EZ_SUPPORTS_MEMORY_MAPPED_FILE EZ_OFF

/// Shared memory IPC is supported.
#undef EZ_SUPPORTS_SHARED_MEMORY
#define EZ_SUPPORTS_SHARED_MEMORY EZ_OFF

/// Whether dynamic plugins (through DLLs loaded/unloaded at runtime) are supported
#undef EZ_SUPPORTS_DYNAMIC_PLUGINS
#define EZ_SUPPORTS_DYNAMIC_PLUGINS EZ_ON

/// Whether applications can access any file (not sandboxed)
#undef EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#define EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS EZ_OFF

/// Whether file accesses can be done through paths that do not match exact casing
#undef EZ_SUPPORTS_CASE_INSENSITIVE_PATHS
#define EZ_SUPPORTS_CASE_INSENSITIVE_PATHS EZ_ON

/// Whether starting other processes is supported.
#undef EZ_SUPPORTS_PROCESSES
#define EZ_SUPPORTS_PROCESSES EZ_OFF

// SIMD support
#undef EZ_SIMD_IMPLEMENTATION

#if EZ_ENABLED(EZ_PLATFORM_ARCH_X86)
#  define EZ_SIMD_IMPLEMENTATION EZ_SIMD_IMPLEMENTATION_SSE
#  define EZ_SSE_LEVEL EZ_SSE_41
#elif EZ_ENABLED(EZ_PLATFORM_ARCH_ARM)
#  define EZ_SIMD_IMPLEMENTATION EZ_SIMD_IMPLEMENTATION_FPU
#else
#  error "Unknown architecture."
#endif

#ifdef _WIN64
#  undef EZ_PLATFORM_64BIT
#  define EZ_PLATFORM_64BIT EZ_ON
#else
#  undef EZ_PLATFORM_32BIT
#  define EZ_PLATFORM_32BIT EZ_ON
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
#endif
