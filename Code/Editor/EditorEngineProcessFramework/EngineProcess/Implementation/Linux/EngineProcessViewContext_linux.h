#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>

namespace
{
  ezResult ezGlfwError(const char* file, size_t line)
  {
    const char* desc;
    int errorCode = glfwGetError(&desc);
    if (errorCode != GLFW_NO_ERROR)
    {
      ezLog::Error("GLFW error {} ({}): {} - {}", file, line, errorCode, desc);
      return EZ_FAILURE;
    }
    return EZ_SUCCESS;
  }
} // namespace

#define EZ_GLFW_RETURN_FAILURE_ON_ERROR()         \
  do                                              \
  {                                               \
    if (ezGlfwError(__FILE__, __LINE__).Failed()) \
      return EZ_FAILURE;                          \
  } while (false)

ezEditorProcessViewWindow::~ezEditorProcessViewWindow()
{
  if (m_hWnd != nullptr)
  {
    glfwDestroyWindow(m_hWnd);
    m_hWnd = nullptr;
  }
}

ezResult ezEditorProcessViewWindow::UpdateWindow(ezWindowHandle parentWindow, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;

  if (m_hWnd == nullptr)
  {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    EZ_GLFW_RETURN_FAILURE_ON_ERROR();

    //glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    //EZ_GLFW_RETURN_FAILURE_ON_ERROR();

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    EZ_GLFW_RETURN_FAILURE_ON_ERROR();

    GLFWwindow* pWindow = glfwCreateWindow(uiWidth, uiHeight, "EditorEngineView", nullptr, nullptr);
    EZ_GLFW_RETURN_FAILURE_ON_ERROR();

    Window xEngineWindow = glfwGetX11Window(pWindow);
    Display* xDisplay = glfwGetX11Display();

    Window xParentWindow = reinterpret_cast<Window>(parentWindow);

    //XSynchronize(xDisplay, true);
    XReparentWindow(xDisplay, xEngineWindow, xParentWindow, 0, 0);

    //glfwShowWindow(pWindow);

    m_hWnd = pWindow;
  }

  return EZ_SUCCESS;
}

// Clean up the mess including Xlib.h caused
#undef None