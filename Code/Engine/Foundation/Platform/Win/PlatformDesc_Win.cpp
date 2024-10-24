#include <Foundation/Platform/PlatformDesc.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezPlatformDesc);

ezPlatformDesc g_PlatformDescWin("Windows", "Desktop");

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

const ezPlatformDesc* ezPlatformDesc::s_pThisPlatform = &g_PlatformDescWin;

#endif
