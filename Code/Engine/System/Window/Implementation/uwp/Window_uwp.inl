#include <System/PCH.h>
#include <Foundation/Basics.h>
#include <System/Basics.h>
#include <System/Window/Window.h>
#include <Foundation/Logging/Log.h>

#include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#include <windows.ui.core.h>
#include <windows.applicationmodel.core.h>

namespace
{
  struct ezWindowUwpData
  {
    ComPtr<ABI::Windows::UI::Core::ICoreDispatcher> m_dispatcher;
    ComPtr<ABI::Windows::UI::Core::ICoreWindow> m_coreWindow;
  };
  ezWindowUwpData* s_uwpWindowData = nullptr;
}

ezResult ezWindow::Initialize()
{
  EZ_LOG_BLOCK("ezWindow::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
    Destroy();

  EZ_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");

  // The main window is handled in a special way in UWP (closing it closes the application, not created explicitely, every window has a thread, ...)
  // which is why we're supporting only a single window for for now.
  EZ_ASSERT_RELEASE(s_uwpWindowData == nullptr, "Currently, there is only a single UWP window supported!");

  
  s_uwpWindowData = EZ_DEFAULT_NEW(ezWindowUwpData);

  ComPtr<ABI::Windows::ApplicationModel::Core::ICoreImmersiveApplication> application;
  EZ_SUCCEED_OR_RETURN(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(), &application));

  ComPtr<ABI::Windows::ApplicationModel::Core::ICoreApplicationView> mainView;
  EZ_SUCCEED_OR_RETURN(application->get_MainView(&mainView));

  EZ_SUCCEED_OR_RETURN(mainView->get_CoreWindow(&s_uwpWindowData->m_coreWindow));
  m_WindowHandle = s_uwpWindowData->m_coreWindow.Get();

  EZ_SUCCEED_OR_RETURN(s_uwpWindowData->m_coreWindow->Activate());
  EZ_SUCCEED_OR_RETURN(s_uwpWindowData->m_coreWindow->get_Dispatcher(&s_uwpWindowData->m_dispatcher));
 
  m_pInputDevice = EZ_DEFAULT_NEW(ezStandardInputDevice);
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


  EZ_DEFAULT_DELETE(m_pInputDevice);
  EZ_DEFAULT_DELETE(s_uwpWindowData);
  s_uwpWindowData = nullptr;


  ezLog::Success("Window destroyed.");

  return EZ_SUCCESS;
}

void ezWindow::ProcessWindowMessages()
{
  EZ_ASSERT_RELEASE(s_uwpWindowData != nullptr, "No uwp window data available.");

  HRESULT result = s_uwpWindowData->m_dispatcher->ProcessEvents(ABI::Windows::UI::Core::CoreProcessEventsOption_ProcessAllIfPresent);
  if (FAILED(result))
  {
    ezLog::Error("Window event processing failed with error code: {0}", result);
  }
}
