
#if EZ_ENABLED(EZ_SUPPORTS_GLFW)

#  include <Core/Platform/GLFW/Window_GLFW.h>

#else

// can't use a 'using' here, because that can't be forward declared
class EZ_CORE_DLL ezWindow : public ezWindowPlatformShared
{
};

#endif
