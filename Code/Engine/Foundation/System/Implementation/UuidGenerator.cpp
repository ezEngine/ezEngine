
#include <Foundation/PCH.h>
#include <Foundation/Types/Uuid.h>

// Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#include <Foundation/System/Implementation/Win/UuidGenerator_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
#include <Foundation/System/Implementation/Posix/UuidGenerator_posix.h>
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
#include <Foundation/System/Implementation/Posix/UuidGenerator_posix.h>
#else
#error "Uuid generation functions are not implemented on current platform"
#endif

EZ_STATICLINK_FILE(Foundation, Foundation_System_Implementation_UuidGenerator);