#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX)
#  if EZ_ENABLED(EZ_USE_POSIX_FILE_API)
#    include <Foundation/Platform/Posix/MemoryMappedFile_Posix.h>
#  else
#    include <Foundation/Platform/NoImpl/MemoryMappedFile_NoImpl.h>
#  endif
#endif
