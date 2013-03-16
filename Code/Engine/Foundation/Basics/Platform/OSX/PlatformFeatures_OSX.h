#pragma once

/// If set to 1, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#define EZ_USE_POSIX_FILE_API 1

/// Iterating through the file system is supported
#define EZ_SUPPORTS_FILE_ITERATORS 0

/// Getting the stats of a file (modification times etc.) is supported.
#define EZ_SUPPORTS_FILE_STATS 0