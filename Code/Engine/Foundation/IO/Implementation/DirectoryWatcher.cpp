#include <Foundation/PCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#include <Foundation/IO/Implementation/Win/DirectoryWatcher_win.h>
#elif EZ_ENABLED(EZ_USE_POSIX_FILE_API)
#include <Foundation/IO/Implementation/Posix/DirectoryWatcher_posix.h>
#else
#error "Unknown Platform."
#endif