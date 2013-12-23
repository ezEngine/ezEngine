#pragma once

/// \file

/// If set to 1, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#undef EZ_USE_POSIX_FILE_API
#define EZ_USE_POSIX_FILE_API EZ_OFF

/// Iterating through the file system is supported
#undef EZ_SUPPORTS_FILE_ITERATORS
#define EZ_SUPPORTS_FILE_ITERATORS EZ_ON

/// Getting the stats of a file (modification times etc.) is supported.
#undef EZ_SUPPORTS_FILE_STATS
#define EZ_SUPPORTS_FILE_STATS EZ_ON

/// Whether dynamic plugins (through DLLs loaded/unloaded at runtime) are supported
#undef EZ_SUPPORTS_DYNAMIC_PLUGINS
#define EZ_SUPPORTS_DYNAMIC_PLUGINS EZ_ON

