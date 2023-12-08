#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)

#  if EZ_ENABLED(EZ_SUPPORTS_GLFW)
#    include <Foundation/Platform/GLFW/Screen_GLFW.h>
#  else
#    include <Foundation/Platform/NoImpl/Screen_NoImpl.h>
#  endif

#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Platform_Android_Screen_Android);

