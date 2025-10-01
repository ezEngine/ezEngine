#include <Core/CorePCH.h>

#if EZ_ENABLED(EZ_SUPPORTS_GLFW)

#  include <Core/System/Window.h>
#  include <Foundation/Configuration/Startup.h>

#  include <GLFW/glfw3.h>

#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#    ifdef APIENTRY
#      undef APIENTRY
#    endif

#    include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#    define GLFW_EXPOSE_NATIVE_WIN32
#    include <GLFW/glfw3native.h>
#  endif

namespace
{
  void glfwErrorCallback(int errorCode, const char* msg)
  {
    ezLog::Error("GLFW error {}: {}", errorCode, msg);
  }
} // namespace


// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, Window)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    if (!glfwInit())
    {
      const char* szErrorDesc = nullptr;
      int iErrorCode = glfwGetError(&szErrorDesc);
      ezLog::Warning("Failed to initialize glfw. Window and input related functionality will not be available. Error Code {}. GLFW Error Message: {}", iErrorCode, szErrorDesc);
    }
    else
    {
      // Set the error callback after init, so we don't print an error if init fails.
      glfwSetErrorCallback(&glfwErrorCallback);
    }
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

#  define EZ_GLFW_RETURN_FAILURE_ON_ERROR()         \
    do                                              \
    {                                               \
      if (ezGlfwError(__FILE__, __LINE__).Failed()) \
        return EZ_FAILURE;                          \
    } while (false)

ezWindowGLFW::~ezWindowGLFW()
{
  DestroyWindow();
}

ezResult ezWindowGLFW::InitializeWindow()
{
  EZ_LOG_BLOCK("ezWindow::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
  {
    DestroyWindow();
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
        if (pVideoMode == nullptr)
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
#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
  m_hWindowHandle.type = ezWindowHandle::Type::GLFW;
  m_hWindowHandle.glfwWindow = pWindow;
#  else
  m_hWindowHandle = pWindow;
#  endif

  if (m_CreationDescription.m_Position != ezVec2I32(0x80000000, 0x80000000))
  {
    glfwSetWindowPos(pWindow, m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y);
    EZ_GLFW_RETURN_FAILURE_ON_ERROR();
  }

  glfwSetWindowUserPointer(pWindow, this);
  glfwSetWindowIconifyCallback(pWindow, &ezWindowGLFW::IconifyCallback);
  glfwSetWindowSizeCallback(pWindow, &ezWindowGLFW::SizeCallback);
  glfwSetWindowPosCallback(pWindow, &ezWindowGLFW::PositionCallback);
  glfwSetWindowCloseCallback(pWindow, &ezWindowGLFW::CloseCallback);
  glfwSetWindowFocusCallback(pWindow, &ezWindowGLFW::FocusCallback);
  glfwSetKeyCallback(pWindow, &ezWindowGLFW::KeyCallback);
  glfwSetCharCallback(pWindow, &ezWindowGLFW::CharacterCallback);
  glfwSetCursorPosCallback(pWindow, &ezWindowGLFW::CursorPositionCallback);
  glfwSetMouseButtonCallback(pWindow, &ezWindowGLFW::MouseButtonCallback);
  glfwSetScrollCallback(pWindow, &ezWindowGLFW::ScrollCallback);
  EZ_GLFW_RETURN_FAILURE_ON_ERROR();

#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
  EZ_ASSERT_DEV(m_hWindowHandle.type == ezWindowHandle::Type::GLFW, "not a GLFW handle");
  auto pInput = EZ_DEFAULT_NEW(ezInputDeviceMouseKeyboard_GLFW, m_hWindowHandle.glfwWindow);
#  else
  auto pInput = EZ_DEFAULT_NEW(ezInputDeviceMouseKeyboard_Win, m_hWindowHandle);
#  endif

  pInput->SetClipMouseCursor(m_CreationDescription.m_bClipMouseCursor ? ezMouseCursorClipMode::ClipToWindowImmediate : ezMouseCursorClipMode::NoClip);
  pInput->SetShowMouseCursor(m_CreationDescription.m_bShowMouseCursor);

  m_pInputDevice = std::move(pInput);

  m_bInitialized = true;
  ezLog::Success("Created glfw window successfully. Resolution is {0}*{1}", GetClientAreaSize().width, GetClientAreaSize().height);

  return EZ_SUCCESS;
}

void ezWindowGLFW::DestroyWindow()
{
  if (m_bInitialized)
  {
    EZ_LOG_BLOCK("ezWindow::Destroy");

    m_pInputDevice = nullptr;

#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
    EZ_ASSERT_DEV(m_hWindowHandle.type == ezWindowHandle::Type::GLFW, "GLFW handle expected");
    glfwDestroyWindow(m_hWindowHandle.glfwWindow);
#  else
    glfwDestroyWindow(m_hWindowHandle);
#  endif
    m_hWindowHandle = INVALID_INTERNAL_WINDOW_HANDLE_VALUE;

    m_bInitialized = false;
  }
}

ezResult ezWindowGLFW::Resize(const ezSizeU32& newWindowSize)
{
  if (!m_bInitialized)
    return EZ_FAILURE;

#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
  EZ_ASSERT_DEV(m_hWindowHandle.type == ezWindowHandle::Type::GLFW, "Expected GLFW handle");
  glfwSetWindowSize(m_hWindowHandle.glfwWindow, newWindowSize.width, newWindowSize.height);
#  else
  glfwSetWindowSize(m_hWindowHandle, newWindowSize.width, newWindowSize.height);
#  endif
  EZ_GLFW_RETURN_FAILURE_ON_ERROR();

  return EZ_SUCCESS;
}

void ezWindowGLFW::ProcessWindowMessages()
{
  if (!m_bInitialized)
    return;

  // Only run the global event processing loop for the main window.
  // if (m_CreationDescription.m_uiWindowNumber == 0)
  {
    glfwPollEvents();
  }

#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
  EZ_ASSERT_DEV(m_hWindowHandle.type == ezWindowHandle::Type::GLFW, "Expected GLFW handle");
  if (glfwWindowShouldClose(m_hWindowHandle.glfwWindow))
  {
    DestroyWindow();
  }
#  else
  if (glfwWindowShouldClose(m_hWindowHandle))
  {
    DestroyWindow();
  }
#  endif
}

void ezWindowGLFW::OnResize(const ezSizeU32& newWindowSize)
{
  ezLog::Info("Window resized to ({0}, {1})", newWindowSize.width, newWindowSize.height);
}

void ezWindowGLFW::IconifyCallback(GLFWwindow* window, int iconified)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self)
    self->OnVisibleChange(!iconified);
}

void ezWindowGLFW::SizeCallback(GLFWwindow* window, int width, int height)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self && width > 0 && height > 0)
  {
    self->OnResize(ezSizeU32(static_cast<ezUInt32>(width), static_cast<ezUInt32>(height)));
  }
}

void ezWindowGLFW::PositionCallback(GLFWwindow* window, int xpos, int ypos)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnWindowMove(xpos, ypos);
  }
}

void ezWindowGLFW::CloseCallback(GLFWwindow* window)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnClickClose();
  }
}

void ezWindowGLFW::FocusCallback(GLFWwindow* window, int focused)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnFocus(focused ? true : false);
  }
}

void ezWindowGLFW::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    if (auto pInput = ezDynamicCast<ezInputDeviceMouseKeyboard_GLFW*>(self->GetInputDevice()))
    {
      pInput->OnKey(key, scancode, action, mods);
    }
  }
}

void ezWindowGLFW::CharacterCallback(GLFWwindow* window, unsigned int codepoint)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    if (auto pInput = ezDynamicCast<ezInputDeviceMouseKeyboard_GLFW*>(self->GetInputDevice()))
    {
      pInput->OnCharacter(codepoint);
    }
  }
}

void ezWindowGLFW::CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    if (auto pInput = ezDynamicCast<ezInputDeviceMouseKeyboard_GLFW*>(self->GetInputDevice()))
    {
      pInput->OnCursorPosition(xpos, ypos);
    }
  }
}

void ezWindowGLFW::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    if (auto pInput = ezDynamicCast<ezInputDeviceMouseKeyboard_GLFW*>(self->GetInputDevice()))
    {
      pInput->OnMouseButton(button, action, mods);
    }
  }
}

void ezWindowGLFW::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
  auto self = static_cast<ezWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    if (auto pInput = ezDynamicCast<ezInputDeviceMouseKeyboard_GLFW*>(self->GetInputDevice()))
    {
      pInput->OnScroll(xoffset, yoffset);
    }
  }
}

ezWindowHandle ezWindowGLFW::GetNativeWindowHandle() const
{
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  return ezMinWindows::FromNative<HWND>(glfwGetWin32Window(m_hWindowHandle));
#  else
  return m_hWindowHandle;
#  endif
}

#endif


EZ_STATICLINK_FILE(Core, Core_Platform_GLFW_Window_GLFW);
