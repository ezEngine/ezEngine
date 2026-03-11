#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  define EZ_POSIX_THREAD_SETNAME
#  include <Foundation/Platform/Posix/OSThread_Posix.inl>
#endif
