#include <Foundation/Platform/PlatformDesc.h>

ezPlatformDesc g_PlatformDescAndroid("Android");

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)

const ezPlatformDesc* ezPlatformDesc::s_pThisPlatform = &g_PlatformDescAndroid;

#endif
