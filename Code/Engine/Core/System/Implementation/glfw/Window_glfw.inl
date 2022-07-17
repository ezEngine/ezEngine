#include <Core/System/Window.h>
#include <Foundation/Configuration/Startup.h>

#include <GLFW/glfw3.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  ifdef APIENTRY
#    undef APIENTRY
#endif

# include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  define GLFW_EXPOSE_NATIVE_WIN32
#  include <GLFW/glfw3native.h>
#endif

namespace
{
  void glfwErrorCallback(int errorCode, const char* msg)
  {
    ezLog::Error("GLFW error {}: {}", errorCode, msg);
  }
}


// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, Window)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    if(!glfwInit())
    {
      const char* szErrorDesc = nullptr;
      int iErrorCode = glfwGetError(&szErrorDesc);
      ezLog::Warning("Failed to initialize glfw. Window and input related functionality will not be available. Error Code {}. GLFW Error Message: {}", iErrorCode, szErrorDesc);
    }
    // Set the error callback after init, so we don't print an error if init fails.
    glfwSetErrorCallback(&glfwErrorCallback);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    glfwSetErrorCallback(nullptr);
    glfwTerminate();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace {
  ezResult ezGlfwError(const char* file, size_t line)
  {
    const char* desc;
    int errorCode = glfwGetError(&desc);
    if(errorCode != GLFW_NO_ERROR)
    {
      ezLog::Error("GLFW error {} ({}): {} - {}", file, line, errorCode, desc);
      return EZ_FAILURE;
    }
    return EZ_SUCCESS;
  }
}

#define EZ_GLFW_RETURN_FAILURE_ON_ERROR() do { if(ezGlfwError(__FILE__, __LINE__).Failed()) return EZ_FAILURE; } while(false)

ezResult ezWindow::Initialize()
{
  EZ_LOG_BLOCK("ezWindow::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
  {
    Destroy().IgnoreResult();
  }

  EZ_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");

  GLFWmonitor* pMonitor = nullptr; // nullptr for windowed, fullscreen otherwise

  switch (m_CreationDescription.m_WindowMode)
  {
    case ezWindowMode::WindowResizable:
      glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
      EZ_GLFW_RETURN_FAILURE_ON_ERROR();
      break;
    case ezWindowMode::WindowFixedResolution:
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
      EZ_GLFW_RETURN_FAILURE_ON_ERROR();
      break;
    case ezWindowMode::FullscreenFixedResolution:
    case ezWindowMode::FullscreenBorderlessNativeResolution:
      if (m_CreationDescription.m_iMonitor == -1)
      {
        pMonitor = glfwGetPrimaryMonitor();
        EZ_GLFW_RETURN_FAILURE_ON_ERROR();
      }
      else
      {
        int iMonitorCount = 0;
        GLFWmonitor** pMonitors = glfwGetMonitors(&iMonitorCount);
        EZ_GLFW_RETURN_FAILURE_ON_ERROR();
        if (m_CreationDescription.m_iMonitor >= iMonitorCount)
        {
          ezLog::Error("Can not create window on monitor {} only {} monitors connected", m_CreationDescription.m_iMonitor, iMonitorCount);
          return EZ_FAILURE;
        }
        pMonitor = pMonitors[m_CreationDescription.m_iMonitor];
      }

      if (m_CreationDescription.m_WindowMode == ezWindowMode::FullscreenBorderlessNativeResolution)
      {
        const GLFWvidmode* pVideoMode = glfwGetVideoMode(pMonitor);
        EZ_GLFW_RETURN_FAILURE_ON_ERROR();
        if(pVideoMode == nullptr)
        {
          ezLog::Error("Failed to get video mode for monitor");
          return EZ_FAILURE;
        }
        m_CreationDescription.m_Resolution.width = pVideoMode->width;
        m_CreationDescription.m_Resolution.height = pVideoMode->height;
        m_CreationDescription.m_Position.x = 0;
        m_CreationDescription.m_Position.y = 0;

        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        EZ_GLFW_RETURN_FAILURE_ON_ERROR();
      }

      break;
  }


  glfwWindowHint(GLFW_FOCUS_ON_SHOW, m_CreationDescription.m_bSetForegroundOnInit ? GLFW_TRUE : GLFW_FALSE);
  EZ_GLFW_RETURN_FAILURE_ON_ERROR();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  EZ_GLFW_RETURN_FAILURE_ON_ERROR();

  GLFWwindow* pWindow = glfwCreateWindow(m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height, m_CreationDescription.m_Title.GetData(), pMonitor, NULL);
  EZ_GLFW_RETURN_FAILURE_ON_ERROR();

  if (pWindow == nullptr)
  {
    ezLog::Error("Failed to create glfw window");
    return EZ_FAILURE;
  }
  m_WindowHandle = pWindow;

  if (m_CreationDescription.m_Position != ezVec2I32(0x80000000, 0x80000000))
  {
    glfwSetWindowPos(pWindow, m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y);
    EZ_GLFW_RETURN_FAILURE_ON_ERROR();
  }

  glfwSetWindowUserPointer(pWindow, this);
  glfwSetWindowSizeCallback(pWindow, &ezWindow::SizeCallback);
  glfwSetWindowPosCallback(pWindow, &ezWindow::PositionCallback);
  glfwSetWindowCloseCallback(pWindow, &ezWindow::CloseCallback);
  glfwSetWindowFocusCallback(pWindow, &ezWindow::FocusCallback);
  glfwSetKeyCallback(pWindow, &ezWindow::KeyCallback);
  glfwSetCharCallback(pWindow, &ezWindow::CharacterCallback);
  glfwSetCursorPosCallback(pWindow, &ezWindow::CursorPositionCallback);
  glfwSetMouseButtonCallback(pWindow, &ezWindow::MouseButtonCallback);
  glfwSetScrollCallback(pWindow, &ezWindow::ScrollCallback);
  EZ_GLFW_RETURN_FAILURE_ON_ERROR();

  m_pInputDevice = EZ_DEFAULT_NEW(ezStandardInputDevice, m_CreationDescription.m_uiWindowNumber, m_WindowHandle);
  m_pInputDevice->SetClipMouseCursor(m_CreationDescription.m_bClipMouseCursor ? ezMouseCursorClipMode::ClipToWindowImmediate : ezMouseCursorClipMode::NoClip);
  m_pInputDevice->SetShowMouseCursor(m_CreationDescription.m_bShowMouseCursor);

  m_bInitialized = true;
  ezLog::Success("Created glfw window successfully. Resolution is {0}*{1}", GetClientAreaSize().width, GetClientAreaSize().height);

  return EZ_SUCCESS;
}

ezResult ezWindow::Destroy()
{
  if (m_bInitialized)
  {
    EZ_LOG_BLOCK("ezWindow::Destroy");

    m_pInputDevice = nullptr;

    glfwDestroyWindow(m_WindowHandle);
    m_WindowHandle = nullptr;

    m_bInitialized = false;
  }

  return EZ_SUCCESS;
}

ezResult ezWindow::Resize(const ezSizeU32& newWindowSize)
{
  if (!m_bInitialized)
    return EZ_FAILURE;

  glfwSetWindowSize(m_WindowHandle, newWindowSize.width, newWindowSize.height);
  EZ_GLFW_RETURN_FAILURE_ON_ERROR();

  return EZ_SUCCESS;
}

void ezWindow::ProcessWindowMessages()
{
  if (!m_bInitialized)
    return;

  // Only run the global event processing loop for the main window.
  if (m_CreationDescription.m_uiWindowNumber == 0)
  {
    glfwPollEvents();
  }

  if (glfwWindowShouldClose(m_WindowHandle))
  {
    Destroy().IgnoreResult();
  }
}

void ezWindow::OnResize(const ezSizeU32& newWindowSize)
{
  ezLog::Info("Window resized to ({0}, {1})", newWindowSize.width, newWindowSize.height);
}

void ezWindow::SizeCallback(GLFWwindow* window, int width, int height)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self && width > 0 && height > 0)
  {
    self->OnResize(ezSizeU32(static_cast<ezUInt32>(width), static_cast<ezUInt32>(height)));
  }
}

void ezWindow::PositionCallback(GLFWwindow* window, int xpos, int ypos)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnWindowMove(xpos, ypos);
  }
}

void ezWindow::CloseCallback(GLFWwindow* window)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnClickClose();
  }
}

void ezWindow::FocusCallback(GLFWwindow* window, int focused)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnFocus(focused ? true : false);
  }
}

void ezWindow::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self && self->m_pInputDevice)
  {
    self->m_pInputDevice->OnKey(key, scancode, action, mods);
  }
}

void ezWindow::CharacterCallback(GLFWwindow* window, unsigned int codepoint)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self && self->m_pInputDevice)
  {
    self->m_pInputDevice->OnCharacter(codepoint);
  }
}

void ezWindow::CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self && self->m_pInputDevice)
  {
    self->m_pInputDevice->OnCursorPosition(xpos, ypos);
  }
}

void ezWindow::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self && self->m_pInputDevice)
  {
    self->m_pInputDevice->OnMouseButton(button, action, mods);
  }
}

void ezWindow::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self && self->m_pInputDevice)
  {
    self->m_pInputDevice->OnScroll(xoffset, yoffset);
  }
}

ezWindowHandle ezWindow::GetNativeWindowHandle() const
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  return ezMinWindows::FromNative<HWND>(glfwGetWin32Window(m_WindowHandle));
#else
  return m_WindowHandle;
#endif
}
