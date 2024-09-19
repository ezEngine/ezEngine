#include <Foundation/Platform/PlatformDesc.h>

ezPlatformDesc g_PlatformDescWeb("Web");

#if EZ_ENABLED(EZ_PLATFORM_WEB)

const ezPlatformDesc* ezPlatformDesc::s_pThisPlatform = &g_PlatformDescWeb;

#endif
