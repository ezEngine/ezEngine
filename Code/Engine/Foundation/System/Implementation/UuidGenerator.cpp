#include <FoundationPCH.h>

#include <Foundation/Types/Uuid.h>


// Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/System/Implementation/Win/UuidGenerator_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
#  include <Foundation/System/Implementation/Posix/UuidGenerator_posix.h>
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <Foundation/System/Implementation/Posix/UuidGenerator_posix.h>
#elif EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/System/Implementation/Android/UuidGenerator_android.h>
#else
#  error "Uuid generation functions are not implemented on current platform"
#endif

ezUuid ezUuid::StableUuidForString(const char* szString)
{
  size_t length = std::strlen(szString);

  ezUuid NewUuid;
  NewUuid.m_uiLow = ezHashingUtils::xxHash64(szString, length);
  NewUuid.m_uiHigh = ezHashingUtils::xxHash64(szString, length, 0x7FFFFFFFFFFFFFE7u);

  return NewUuid;
}

ezUuid ezUuid::StableUuidForInt(ezInt64 iInt)
{
  ezUuid NewUuid;
  NewUuid.m_uiLow = ezHashingUtils::xxHash64(&iInt, sizeof(ezInt64));
  NewUuid.m_uiHigh = ezHashingUtils::xxHash64(&iInt, sizeof(ezInt64), 0x7FFFFFFFFFFFFFE7u);

  return NewUuid;
}

EZ_STATICLINK_FILE(Foundation, Foundation_System_Implementation_UuidGenerator);
