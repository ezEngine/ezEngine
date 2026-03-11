
#if EZ_ENABLED(EZ_SUPPORTS_GLFW)

#  include <Core/Platform/GLFW/InputDevice_GLFW.h>

extern "C"
{
  typedef struct GLFWwindow GLFWwindow;
}

extern "C"
{
  typedef struct xcb_connection_t xcb_connection_t;
}

struct ezXcbWindowHandle
{
  xcb_connection_t* m_pConnection;
  ezUInt32 m_Window;
};

struct ezWindowHandle
{
  enum class Type
  {
    Invalid = 0,
    GLFW = 1, // Used by the runtime
    XCB = 2   // Used by the editor
  };

  Type type;
  union
  {
    GLFWwindow* glfwWindow;
    ezXcbWindowHandle xcbWindow;
  };

  bool operator==(ezWindowHandle& rhs)
  {
    if (type != rhs.type)
      return false;

    if (type == Type::GLFW)
    {
      return glfwWindow == rhs.glfwWindow;
    }
    else
    {
      // We don't compare the connection because we only want to know if we reference the same window.
      return xcbWindow.m_Window == rhs.xcbWindow.m_Window;
    }
  }
};

using ezWindowInternalHandle = ezWindowHandle;
#  define INVALID_WINDOW_HANDLE_VALUE \
    ezWindowHandle {}

#  define INVALID_INTERNAL_WINDOW_HANDLE_VALUE INVALID_WINDOW_HANDLE_VALUE

#else

#  error "Linux has no native window support"

#endif
