#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_OSX)

#  if EZ_ENABLED(EZ_SUPPORTS_GLFW)
#    include <Foundation/Platform/GLFW/Screen_GLFW.h>
#  else
#    include <Foundation/Platform/NoImpl/Screen_NoImpl.h>
#  endif

#endif
