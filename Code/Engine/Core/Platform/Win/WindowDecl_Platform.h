
#if EZ_ENABLED(EZ_SUPPORTS_GLFW)

#  include <Core/Platform/GLFW/InputDevice_GLFW.h>
#  include <Foundation/Platform/Win/Utils/MinWindows.h>

extern "C"
{
  typedef struct GLFWwindow GLFWwindow;
}

using ezWindowHandle = ezMinWindows::HWND;
using ezWindowInternalHandle = GLFWwindow*;
#  define INVALID_WINDOW_HANDLE_VALUE (ezWindowHandle)(0)
#  define INVALID_INTERNAL_WINDOW_HANDLE_VALUE nullptr

#else

#  include <Foundation/Platform/Win/Utils/MinWindows.h>
#  include <InputDevice_Platform.h>

using ezWindowHandle = ezMinWindows::HWND;
using ezWindowInternalHandle = ezWindowHandle;
#  define INVALID_WINDOW_HANDLE_VALUE (ezWindowHandle)(0)

#endif
