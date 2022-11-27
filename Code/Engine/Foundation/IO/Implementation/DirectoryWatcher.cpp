#include <Foundation/FoundationPCH.h>


#if EZ_ENABLED(EZ_SUPPORTS_DIRECTORY_WATCHER)
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#    include <Foundation/IO/Implementation/Win/DirectoryWatcher_win.h>
#  elif EZ_ENABLED(EZ_USE_POSIX_FILE_API)
#    include <Foundation/IO/Implementation/Posix/DirectoryWatcher_posix.h>
#  else
#    error "Unknown Platform."
#  endif
#endif


EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_DirectoryWatcher);
