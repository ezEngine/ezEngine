#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_OSX)
#  if (EZ_ENABLED(EZ_SUPPORTS_DIRECTORY_WATCHER) && EZ_ENABLED(EZ_USE_POSIX_FILE_API))
#    include <Foundation/Platform/Posix/DirectoryWatcher_Posix.h>
#  else
#    error "DirectoryWatcher not implemented."
#  endif
#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Platform_OSX_DirectoryWatcher_OSX);

