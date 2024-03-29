#include <Core/CorePCH.h>

#include <Core/System/Window.h>
#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#include <android_native_app_glue.h>

struct ANativeWindow;

namespace
{
  ANativeWindow* s_androidWindow = nullptr;
} // namespace

ezResult ezWindow::Initialize()
{
  EZ_LOG_BLOCK("ezWindow::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
  {
    Destroy().IgnoreResult();
  }

  // Checking and adjustments to creation desc.
  {
//    if (m_CreationDescription.m_WindowMode == ezWindowMode::WindowFixedResolution)
//    {
//      ezLog::Warning("Only Fullscreen modes are supported on Android. Falling back to ezWindowMode::FullscreenFixedResolution.");
//      m_CreationDescription.m_WindowMode = ezWindowMode::FullscreenFixedResolution;
//    }
//    else if (m_CreationDescription.m_WindowMode == ezWindowMode::WindowResizable)
//    {
//      ezLog::Warning("Only FullscreenFixedResolution and FullscreenBorderlessNativeResolution modes are supported on Android. Falling back to ezWindowMode::FullscreenBorderlessNativeResolution.");
//      m_CreationDescription.m_WindowMode = ezWindowMode::FullscreenBorderlessNativeResolution;
//    }

    if (m_CreationDescription.AdjustWindowSizeAndPosition().Failed())
      ezLog::Warning("Failed to adjust window size and position settings.");

    EZ_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");
  }

  EZ_ASSERT_RELEASE(s_androidWindow == nullptr, "Window already exists. Only one Android window is supported at any time!");

  s_androidWindow = ezAndroidUtils::GetAndroidApp()->window;
  m_hWindowHandle = s_androidWindow;
  m_pInputDevice = EZ_DEFAULT_NEW(ezStandardInputDevice, 0);
  m_bInitialized = true;

  return EZ_SUCCESS;
}

ezResult ezWindow::Destroy()
{
  if (!m_bInitialized)
    return EZ_SUCCESS;

  EZ_LOG_BLOCK("ezWindow::Destroy");

  s_androidWindow = nullptr;


  ezLog::Success("Window destroyed.");

  return EZ_SUCCESS;
}

ezResult ezWindow::Resize(const ezSizeU32& newWindowSize)
{
  // No need to resize on Android, swapchain can take any size at any time.
  return EZ_SUCCESS;
}

void ezWindow::ProcessWindowMessages()
{
  EZ_ASSERT_RELEASE(s_androidWindow != nullptr, "No uwp window data available.");
}

void ezWindow::OnResize(const ezSizeU32& newWindowSize)
{
  ezLog::Info("Window resized to ({0}, {1})", newWindowSize.width, newWindowSize.height);
}

ezWindowHandle ezWindow::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}
