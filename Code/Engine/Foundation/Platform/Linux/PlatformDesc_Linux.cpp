#include <Foundation/Platform/PlatformDesc.h>

ezPlatformDesc g_PlatformDescLinux("Linux");

#if EZ_ENABLED(EZ_PLATFORM_LINUX)

const ezPlatformDesc* ezPlatformDesc::s_pThisPlatform = &g_PlatformDescLinux;

#endif
