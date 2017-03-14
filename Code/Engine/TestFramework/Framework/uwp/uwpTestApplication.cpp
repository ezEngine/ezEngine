#include <PCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#include <TestFramework/Framework/uwp/uwpTestApplication.h>
#include <TestFramework/Framework/uwp/uwpTestFramework.h>

#include <windows.ui.core.h>
#include <wrl/event.h>

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

HRESULT ezUwpTestApplication::Initialize(ICoreApplicationView* applicationView)
{
  using OnActivatedHandler = __FITypedEventHandler_2_Windows__CApplicationModel__CCore__CCoreApplicationView_Windows__CApplicationModel__CActivation__CIActivatedEventArgs;
  EZ_SUCCEED_OR_RETURN(applicationView->add_Activated(Callback<OnActivatedHandler>(this, &ezUwpTestApplication::OnActivated).Get(), &m_eventRegistrationOnActivate));

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

HRESULT ezUwpTestApplication::OnActivated(ICoreApplicationView* applicationView, IActivatedEventArgs* args)
{
  applicationView->remove_Activated(m_eventRegistrationOnActivate);

  ActivationKind activationKind;
  EZ_SUCCEED_OR_RETURN(args->get_Kind(&activationKind));

  if (activationKind == ActivationKind_Launch)
  {
    ComPtr<ILaunchActivatedEventArgs> launchArgs;
    EZ_SUCCEED_OR_RETURN(args->QueryInterface(launchArgs.GetAddressOf()));

    HString argHString;
    EZ_SUCCEED_OR_RETURN(launchArgs->get_Arguments(argHString.GetAddressOf()));

    ezDynamicArray<ezString> commandLineArgs;
    ezDynamicArray<const char*> argv;
    ezCommandLineUtils::SplitCommandLineString(ezStringUtf8(argHString).GetData(), true, commandLineArgs, argv);

    m_testFramework.GetTestSettingsFromCommandLine(argv.GetCount(), argv.GetData());
  }

  return S_OK;
}

#endif

EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_uwp_uwpTestFramework);

