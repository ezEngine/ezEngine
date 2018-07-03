#include <PCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#include <Foundation/Strings/StringConversion.h>
#include <TestFramework/Framework/uwp/uwpTestApplication.h>
#include <TestFramework/Framework/uwp/uwpTestFramework.h>
#include <windows.ui.core.h>
#include <wrl/event.h>

using namespace ABI::Windows::Foundation;

ezUwpTestApplication::ezUwpTestApplication(ezTestFramework& testFramework)
    : m_testFramework(testFramework)
{
}

ezUwpTestApplication::~ezUwpTestApplication() {}

HRESULT ezUwpTestApplication::CreateView(IFrameworkView** viewProvider)
{
  *viewProvider = this;
  return S_OK;
}

HRESULT ezUwpTestApplication::Initialize(ICoreApplicationView* applicationView)
{
  using OnActivatedHandler =
      __FITypedEventHandler_2_Windows__CApplicationModel__CCore__CCoreApplicationView_Windows__CApplicationModel__CActivation__CIActivatedEventArgs;
  EZ_SUCCEED_OR_RETURN(applicationView->add_Activated(Callback<OnActivatedHandler>(this, &ezUwpTestApplication::OnActivated).Get(),
                                                      &m_eventRegistrationOnActivate));



  ezStartup::StartupBase();

  return S_OK;
}

HRESULT ezUwpTestApplication::SetWindow(ABI::Windows::UI::Core::ICoreWindow* window)
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
  EZ_SUCCEED_OR_RETURN(
      ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Core_CoreWindow).Get(), &coreWindowStatics));
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

    // Setup an extended execution session to prevent app from going to sleep during testing.
    ezUwpUtils::CreateInstance<IExtendedExecutionSession>(RuntimeClass_Windows_ApplicationModel_ExtendedExecution_ExtendedExecutionSession,
                                                          m_extendedExecutionSession);
    EZ_ASSERT_DEV(m_extendedExecutionSession, "Failed to create extended session. Can't prevent app from backgrounding during testing.");
    m_extendedExecutionSession->put_Reason(ExtendedExecutionReason::ExtendedExecutionReason_Unspecified);
    ezStringHString desc("Keep Unit Tests Running");
    m_extendedExecutionSession->put_Description(desc.GetData().Get());

    using OnRevokedHandler =
        __FITypedEventHandler_2_IInspectable_Windows__CApplicationModel__CExtendedExecution__CExtendedExecutionRevokedEventArgs;
    EZ_SUCCEED_OR_RETURN(m_extendedExecutionSession->add_Revoked(
        Callback<OnRevokedHandler>(this, &ezUwpTestApplication::OnSessionRevoked).Get(), &m_eventRegistrationOnRevokedSession));

    ComPtr<__FIAsyncOperation_1_Windows__CApplicationModel__CExtendedExecution__CExtendedExecutionResult> pAsyncOp;
    if (SUCCEEDED(m_extendedExecutionSession->RequestExtensionAsync(&pAsyncOp)))
    {
      ezUwpUtils::ezWinRtPutCompleted<ExtendedExecutionResult, ExtendedExecutionResult>(
          pAsyncOp, [this](const ExtendedExecutionResult& pResult) {
            switch (pResult)
            {
              case ExtendedExecutionResult::ExtendedExecutionResult_Allowed:
                ezLog::Info("Extended session is active.");
                break;
              case ExtendedExecutionResult::ExtendedExecutionResult_Denied:
                ezLog::Error("Extended session is denied.");
                break;
            }
          });
    }
  }

  return S_OK;
}

HRESULT ezUwpTestApplication::OnSessionRevoked(IInspectable* sender, IExtendedExecutionRevokedEventArgs* args)
{
  ezLog::Error("Extended session revoked.");
  return S_OK;
}

#endif

EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_uwp_uwpTestApplication);
