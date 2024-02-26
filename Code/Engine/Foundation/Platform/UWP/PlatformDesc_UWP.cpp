#include <Foundation/Platform/PlatformDesc.h>

ezPlatformDesc g_PlatformDescUWP("UWP");

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

const ezPlatformDesc* ezPlatformDesc::s_pThisPlatform = &g_PlatformDescUWP;

#endif


