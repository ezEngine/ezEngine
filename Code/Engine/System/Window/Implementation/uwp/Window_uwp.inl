#include <PCH.h>
#include <Foundation/Basics.h>
#include <System/Basics.h>
#include <System/Window/Window.h>
#include <Foundation/Logging/Log.h>

#include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#include <windows.ui.core.h>
#include <windows.applicationmodel.core.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  struct ezWindowUwpData
  {
    ComPtr<ABI::Windows::UI::Core::ICoreDispatcher> m_dispatcher;
    ComPtr<ABI::Windows::UI::Core::ICoreWindow> m_coreWindow;
  };
  ezUniquePtr<ezWindowUwpData> s_uwpWindowData;
}

ezResult ezWindow::Initialize()
{
  EZ_LOG_BLOCK("ezWindow::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
    Destroy();

  // Checking and adjustments to creation desc.
  {
    if (m_CreationDescription.m_WindowMode == ezWindowMode::FullscreenFixedResolution)
    {
      ezLog::Warning("ezWindowMode::FullscreenFixedResolution is not supported on UWP. Falling back to ezWindowMode::FullscreenBorderlessNativeResolution.");
      m_CreationDescription.m_WindowMode = ezWindowMode::FullscreenBorderlessNativeResolution;
    }
    else if (m_CreationDescription.m_WindowMode == ezWindowMode::WindowFixedResolution)
    {
      ezLog::Warning("ezWindowMode::WindowFixedResolution is not supported on UWP since resizing a window can not be restricted. Falling back to ezWindowMode::WindowResizable");
      m_CreationDescription.m_WindowMode = ezWindowMode::WindowResizable;
    }

    if (m_CreationDescription.AdjustWindowSizeAndPosition().Failed())
      ezLog::Warning("Failed to adjust window size and position settings.");

    EZ_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");
  }

  // The main window is handled in a special way in UWP (closing it closes the application, not created explicitely, every window has a thread, ...)
  // which is why we're supporting only a single window for for now.
  EZ_ASSERT_RELEASE(s_uwpWindowData == nullptr, "Currently, there is only a single UWP window supported!");

  
  s_uwpWindowData = EZ_DEFAULT_NEW(ezWindowUwpData);

  ComPtr<ABI::Windows::ApplicationModel::Core::ICoreImmersiveApplication> application;
  EZ_HRESULT_TO_FAILURE(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(), &application));

  ComPtr<ABI::Windows::ApplicationModel::Core::ICoreApplicationView> mainView;
  EZ_HRESULT_TO_FAILURE(application->get_MainView(&mainView));

  EZ_HRESULT_TO_FAILURE(mainView->get_CoreWindow(&s_uwpWindowData->m_coreWindow));
  m_WindowHandle = s_uwpWindowData->m_coreWindow.Get();

  // Activation of main window already done in Uwp application implementation.
//  EZ_HRESULT_TO_FAILURE(s_uwpWindowData->m_coreWindow->Activate());
  EZ_HRESULT_TO_FAILURE(s_uwpWindowData->m_coreWindow->get_Dispatcher(&s_uwpWindowData->m_dispatcher));

  {
    // Get current *logical* screen DPI to do a pixel correct resize.
    ComPtr<ABI::Windows::Graphics::Display::IDisplayInformationStatics> displayInfoStatics;
    EZ_HRESULT_TO_FAILURE(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(), &displayInfoStatics));
    ComPtr<ABI::Windows::Graphics::Display::IDisplayInformation> displayInfo;
    EZ_HRESULT_TO_FAILURE(displayInfoStatics->GetForCurrentView(&displayInfo));
    FLOAT logicalDpi = 1.0f;
    EZ_HRESULT_TO_FAILURE(displayInfo->get_LogicalDpi(&logicalDpi));

    // Need application view for the next steps...
    ComPtr<ABI::Windows::UI::ViewManagement::IApplicationViewStatics2> appViewStatics;
    EZ_HRESULT_TO_FAILURE(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_ViewManagement_ApplicationView).Get(), &appViewStatics));
    ComPtr<ABI::Windows::UI::ViewManagement::IApplicationView> appView;
    EZ_HRESULT_TO_FAILURE(appViewStatics->GetForCurrentView(&appView));
    ComPtr<ABI::Windows::UI::ViewManagement::IApplicationView2> appView2;
    EZ_HRESULT_TO_FAILURE(appView.As(&appView2));
    ComPtr<ABI::Windows::UI::ViewManagement::IApplicationView3> appView3;
    EZ_HRESULT_TO_FAILURE(appView.As(&appView3));

    // Request/remove fullscreen from window if requested.
    boolean isFullscreen;
    EZ_HRESULT_TO_FAILURE(appView3->get_IsFullScreenMode(&isFullscreen));
    if ((isFullscreen > 0) != ezWindowMode::IsFullscreen(m_CreationDescription.m_WindowMode))
    {
      if (ezWindowMode::IsFullscreen(m_CreationDescription.m_WindowMode))
      {
        EZ_HRESULT_TO_FAILURE(appView3->TryEnterFullScreenMode(&isFullscreen));
        if (!isFullscreen)
          ezLog::Warning("Failed to enter full screen mode.");
      }
      else
      {
        EZ_HRESULT_TO_FAILURE(appView3->ExitFullScreenMode());
      }
    }

    // Set size. Pointless though if we're fullscreen.
    if (!isFullscreen)
    {
      boolean successfulResize = false;
      ABI::Windows::Foundation::Size size;
      size.Width = m_CreationDescription.m_Resolution.width * 96.0f / logicalDpi;
      size.Height = m_CreationDescription.m_Resolution.height * 96.0f / logicalDpi;
      EZ_HRESULT_TO_FAILURE(appView3->TryResizeView(size, &successfulResize));
      if (!successfulResize)
      {
        ABI::Windows::Foundation::Rect visibleBounds;
        EZ_HRESULT_TO_FAILURE(appView2->get_VisibleBounds(&visibleBounds));
        ezUInt32 actualWidth = static_cast<ezUInt32>(visibleBounds.Width * (logicalDpi / 96.0f));
        ezUInt32 actualHeight = static_cast<ezUInt32>(visibleBounds.Height * (logicalDpi / 96.0f));

        ezLog::Warning("Failed to resize the window to {0}x{1}, instead (visible) size remains at {2}x{3}",
            m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height, actualWidth, actualHeight);

        //m_CreationDescription.m_Resolution.width = actualWidth;
        //m_CreationDescription.m_Resolution.height = actualHeight;
      }
    }
  }
 
  m_pInputDevice = EZ_DEFAULT_NEW(ezStandardInputDevice, s_uwpWindowData->m_coreWindow.Get());
  m_pInputDevice->SetClipMouseCursor(m_CreationDescription.m_bClipMouseCursor);
  m_pInputDevice->SetShowMouseCursor(m_CreationDescription.m_bShowMouseCursor);

  m_bInitialized = true;

  return EZ_SUCCESS;
}

ezResult ezWindow::Destroy()
{
  if (!m_bInitialized)
    return EZ_SUCCESS;

  EZ_LOG_BLOCK("ezWindow::Destroy");

  m_pInputDevice = nullptr;
  s_uwpWindowData = nullptr;


  ezLog::Success("Window destroyed.");

  return EZ_SUCCESS;
}

void ezWindow::ProcessWindowMessages()
{
  EZ_ASSERT_RELEASE(s_uwpWindowData != nullptr, "No uwp window data available.");

  // Apparently ProcessAllIfPresent does NOT process all events in the queue somehow
  // if this isn't executed quite often every frame (even at 60 Hz), spatial input events quickly queue up
  // and are delivered with many seconds delay
  // as far as I can tell, there is no way to figure out whether there are more queued events
  for (int i = 0; i < 64; ++i)
  {
    HRESULT result = s_uwpWindowData->m_dispatcher->ProcessEvents(ABI::Windows::UI::Core::CoreProcessEventsOption_ProcessAllIfPresent);
    if (FAILED(result))
    {
      ezLog::Error("Window event processing failed with error code: {0}", result);
    }
  }
}

void ezWindow::OnResize(const ezSizeU32& newWindowSize)
{
  ezLog::Info("Window resized to ({0}, {1})", newWindowSize.width, newWindowSize.height);
}


