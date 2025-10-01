#include <Core/CorePCH.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)

#  include <Core/System/Window.h>
#  include <Foundation/Basics.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Platform/Android/Utils/AndroidUtils.h>
#  include <Foundation/System/Screen.h>
#  include <Foundation/Types/UniquePtr.h>
#  include <android_native_app_glue.h>

struct ANativeWindow;

namespace
{
  ANativeWindow* s_androidWindow = nullptr;
  ezEventSubscriptionID s_androidCommandID = 0;
} // namespace

ezWindowAndroid::~ezWindowAndroid()
{
  DestroyWindow();
}

ezResult ezWindowAndroid::InitializeWindow()
{
  EZ_LOG_BLOCK("ezWindow::Initialize", m_CreationDescription.m_Title.GetData());
  if (m_bInitialized)
  {
    DestroyWindow();
  }

  if (m_CreationDescription.m_WindowMode == ezWindowMode::WindowResizable)
  {
    s_androidCommandID = ezAndroidUtils::s_AppCommandEvent.AddEventHandler([this](ezInt32 iCmd)
      {
      if (iCmd == APP_CMD_WINDOW_RESIZED)
      {
        ezHybridArray<ezScreenInfo, 2> screens;
        if (ezScreen::EnumerateScreens(screens).Succeeded())
        {
          m_CreationDescription.m_Resolution.width = screens[0].m_iResolutionX;
          m_CreationDescription.m_Resolution.height = screens[0].m_iResolutionY;
          this->OnResize(ezSizeU32(screens[0].m_iResolutionX, screens[0].m_iResolutionY));
        }
      } });
  }

  // Checking and adjustments to creation desc.
  if (m_CreationDescription.AdjustWindowSizeAndPosition().Failed())
    ezLog::Warning("Failed to adjust window size and position settings.");

  EZ_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");
  EZ_ASSERT_RELEASE(s_androidWindow == nullptr, "Window already exists. Only one Android window is supported at any time!");

  s_androidWindow = ezAndroidUtils::GetAndroidApp()->window;
  m_hWindowHandle = s_androidWindow;
  m_pInputDevice = EZ_DEFAULT_NEW(ezInputDevice_Android);
  m_bInitialized = true;

  return EZ_SUCCESS;
}

void ezWindowAndroid::DestroyWindow()
{
  if (!m_bInitialized)
    return;

  EZ_LOG_BLOCK("ezWindow::Destroy");

  s_androidWindow = nullptr;

  if (s_androidCommandID != 0)
  {
    ezAndroidUtils::s_AppCommandEvent.RemoveEventHandler(s_androidCommandID);
  }

  ezLog::Success("Window destroyed.");
}

ezResult ezWindowAndroid::Resize(const ezSizeU32& newWindowSize)
{
  // No need to resize on Android, swapchain can take any size at any time.
  m_CreationDescription.m_Resolution.width = newWindowSize.width;
  m_CreationDescription.m_Resolution.height = newWindowSize.height;
  return EZ_SUCCESS;
}

void ezWindowAndroid::ProcessWindowMessages()
{
  EZ_ASSERT_RELEASE(s_androidWindow != nullptr, "No window data available.");
}

void ezWindowAndroid::OnResize(const ezSizeU32& newWindowSize)
{
  ezLog::Info("Window resized to ({0}, {1})", newWindowSize.width, newWindowSize.height);
}

ezWindowHandle ezWindowAndroid::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}

#endif
