#include <FoundationPCH.h>



#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/IO/Implementation/Win/MemoryMappedFile_win.h>
#elif EZ_ENABLED(EZ_USE_POSIX_FILE_API)
#  include <Foundation/IO/Implementation/Posix/MemoryMappedFile_posix.h>
#else
#  error "Unknown Platform."
#endif
