#include <PCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#include <TestFramework/Framework/uwp/uwpTestApplication.h>
#include <TestFramework/Framework/uwp/uwpTestFramework.h>

#include <windows.ui.core.h>

ezUwpTestApplication::ezUwpTestApplication(ezTestFramework& testFramework)
  : m_testFramework(testFramework)
{
}

ezUwpTestApplication::~ezUwpTestApplication()
{
}

HRESULT ezUwpTestApplication::CreateView(IFrameworkView** viewProvider)
{
  *viewProvider = this;
  return S_OK;
}

HRESULT ezUwpTestApplication::Initialize(ICoreApplicationView * applicationView)
{
  ezStartup::StartupBase();
  return S_OK;
}

HRESULT ezUwpTestApplication::SetWindow(ABI::Windows::UI::Core::ICoreWindow * window)
{
  return S_OK;
}

HRESULT ezUwpTestApplication::Load(HSTRING entryPoint)
{
  return S_OK;
}

HRESULT ezUwpTestApplication::Run()
{
  ComPtr<ABI::Windows::UI::Core::ICoreWindowStatic> coreWindowStatics;
  EZ_SUCCEED_OR_RETURN(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Core_CoreWindow).Get(), &coreWindowStatics));
  ComPtr<ABI::Windows::UI::Core::ICoreWindow> coreWindow;
  EZ_SUCCEED_OR_RETURN(coreWindowStatics->GetForCurrentThread(&coreWindow));
  ComPtr<ABI::Windows::UI::Core::ICoreDispatcher> dispatcher;
  EZ_SUCCEED_OR_RETURN(coreWindow->get_Dispatcher(&dispatcher));

  while (m_testFramework.RunTestExecutionLoop() == ezTestAppRun::Continue)
  {
    dispatcher->ProcessEvents(ABI::Windows::UI::Core::CoreProcessEventsOption_ProcessAllIfPresent);
  }

  return S_OK;
}

HRESULT ezUwpTestApplication::Uninitialize()
{
  m_testFramework.AbortTests();
  return S_OK;
}

#endif

EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_uwp_uwpTestFramework);

