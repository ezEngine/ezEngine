#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  define EZ_POSIX_MMAP_SKIPUNLINK
#  include <Foundation/Platform/Posix/MemoryMappedFile_Posix.inl>
#endif
