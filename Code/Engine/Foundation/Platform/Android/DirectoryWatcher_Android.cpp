#include <Foundation/FoundationPCH.h>

#if (EZ_ENABLED(EZ_PLATFORM_ANDROID) && EZ_ENABLED(EZ_SUPPORTS_DIRECTORY_WATCHER))
#  if EZ_ENABLED(EZ_USE_POSIX_FILE_API)
#    include <Foundation/Platform/Posix/DirectoryWatcher_Posix.h>
#  else
#    error "DirectoryWatcher not implemented."
#  endif
#endif
