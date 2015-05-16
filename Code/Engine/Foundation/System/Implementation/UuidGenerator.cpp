
#include <Foundation/PCH.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Algorithm/Hashing.h>


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

ezUuid ezUuid::StableUuidForString(const char* szString)
{
	size_t length = std::strlen( szString );

	ezUuid NewUuid;
	NewUuid.m_uiLow = ezHashing::MurmurHash64( szString, length );
	NewUuid.m_uiHigh = ezHashing::MurmurHash64( szString, length, 0x7FFFFFFFFFFFFFE7u );

	return NewUuid;
}

EZ_STATICLINK_FILE(Foundation, Foundation_System_Implementation_UuidGenerator);