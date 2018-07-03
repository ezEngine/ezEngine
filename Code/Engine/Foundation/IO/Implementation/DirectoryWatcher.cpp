#include <PCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#include <Foundation/IO/Implementation/Win/DirectoryWatcher_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#include <Foundation/IO/Implementation/Win/DirectoryWatcher_uwp.h>
#elif EZ_ENABLED(EZ_USE_POSIX_FILE_API)
#include <Foundation/IO/Implementation/Posix/DirectoryWatcher_posix.h>
#else
#error "Unknown Platform."
#endif



EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_DirectoryWatcher);
