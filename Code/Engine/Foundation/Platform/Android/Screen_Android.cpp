#include <Foundation/FoundationPCH.h>

#include <Foundation/System/PlatformFeatures.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)

#  if EZ_ENABLED(EZ_SUPPORTS_GLFW)
#    include <Foundation/Platform/GLFW/Screen_GLFW.h>
#  else
#    include <Foundation/Platform/NoImpl/Screen_NoImpl.h>
#  endif

#endif

