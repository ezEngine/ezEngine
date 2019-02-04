#pragma once

/// If set to 1, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#undef EZ_USE_POSIX_FILE_API
#define EZ_USE_POSIX_FILE_API EZ_ON

/// Iterating through the file system is not supported
#undef EZ_SUPPORTS_FILE_ITERATORS
#define EZ_SUPPORTS_FILE_ITERATORS EZ_OFF

/// Getting the stats of a file (modification times etc.) is supported.
#undef EZ_SUPPORTS_FILE_STATS
#define EZ_SUPPORTS_FILE_STATS EZ_ON

/// Whether dynamic plugins (through DLLs loaded/unloaded at runtime) are supported
#undef EZ_SUPPORTS_DYNAMIC_PLUGINS
#define EZ_SUPPORTS_DYNAMIC_PLUGINS EZ_OFF

/// Whether applications can access any file (not sandboxed)
#undef EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#define EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS EZ_ON

/// Whether file accesses can be done through paths that do not match exact casing
#undef EZ_SUPPORTS_CASE_INSENSITIVE_PATHS
#define EZ_SUPPORTS_CASE_INSENSITIVE_PATHS EZ_OFF

// SIMD support
#undef EZ_SIMD_IMPLEMENTATION
#define EZ_SIMD_IMPLEMENTATION EZ_SIMD_IMPLEMENTATION_FPU

